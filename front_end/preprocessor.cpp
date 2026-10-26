#include "my_compiler/preprocessor.hpp"
#include "my_compiler/compilation_error.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace my_compiler::preprocessing
{
    namespace
    {
        namespace fs = std::filesystem;

        std::string trim(const std::string& text)
        {
            const auto first = text.find_first_not_of(" \t\r\n");
            if(first == std::string::npos)
                return {};
            return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
        }

        // 注释只在普通代码中生效；字符串内的//、/*和空白必须保留。
        std::string remove_comments(const std::string& source, const std::string& file)
        {
            enum class State
            {
                Code,
                Quoted,
                LineComment,
                BlockComment
            };
            State state = State::Code;
            char quote = 0;
            bool escaped = false;
            std::size_t line = 1, comment_line = 0;
            std::string result;
            for(std::size_t i = 0; i < source.size(); ++i)
            {
                const char c = source[i];
                const char next = i + 1 < source.size() ? source[i + 1] : 0;
                if(state == State::LineComment)
                {
                    if(c == '\n')
                    {
                        result += c;
                        state = State::Code;
                    }
                }
                else if(state == State::BlockComment)
                {
                    if(c == '*' && next == '/')
                    {
                        ++i;
                        state = State::Code;
                    }
                    else if(c == '\n')
                        result += c;
                }
                else if(state == State::Quoted)
                {
                    result += c;
                    if(escaped)
                        escaped = false;
                    else if(c == '\\')
                        escaped = true;
                    else if(c == quote)
                        state = State::Code;
                }
                else if(c == '/' && (next == '/' || next == '*'))
                {
                    // 注释是分隔符，不能把int/**/value粘成intvalue。
                    result += ' ';
                    comment_line = line;
                    state = next == '/' ? State::LineComment : State::BlockComment;
                    ++i;
                }
                else
                {
                    result += c;
                    if(c == '"' || c == '\'')
                    {
                        quote = c;
                        state = State::Quoted;
                    }
                }
                if(c == '\n')
                    ++line;
            }
            if(state == State::BlockComment)
                throw CompilationError("E_PP_COMMENT", file, comment_line, 0,
                                       "unterminated block comment");
            return result;
        }

        class Processor
        {
        public:
            Result result;
            std::vector<fs::path> dependencies;

            std::string expand(const fs::path& requested)
            {
                std::error_code error;
                const auto path = fs::canonical(requested, error);
                if(error || !fs::is_regular_file(path))
                    throw CompilationError("E_PP_INCLUDE", requested.string(), 0, 0,
                                           "cannot read source or included file");
                const auto key = path.string();
                if(once_.count(key))
                    return "\n";
                const auto guard = guards_.find(key);
                if(guard != guards_.end() && macros_.count(guard->second))
                    return "\n";
                if(active_.count(key))
                    throw CompilationError("E_PP_CYCLE", key, 0, 0,
                                           "unguarded recursive include");
                if(active_.size() >= 32)
                    throw CompilationError("E_PP_LIMIT", key, 0, 0,
                                           "include nesting exceeds 32 files");
                std::ifstream stream(path, std::ios::binary);
                const std::string raw{std::istreambuf_iterator<char>(stream), {}};
                if(!stream || stream.bad())
                    throw CompilationError("E_PP_INCLUDE", key, 0, 0, "cannot read included file");
                bytes_read_ += raw.size();
                if(raw.size() > 1024 * 1024 || bytes_read_ > 8 * 1024 * 1024)
                    throw CompilationError("E_PP_LIMIT", key, 0, 0, "preprocessing input size limit exceeded");
                const auto cleaned = remove_comments(raw, key);
                std::vector<std::string> lines;
                std::istringstream input(cleaned);
                for(std::string line; std::getline(input, line);)
                    lines.push_back(line);

                // 只把完整包围文件的常见#ifndef/#define/#endif形式当作包含保护。
                std::vector<std::string> significant;
                for(const auto& line : lines)
                    if(!trim(line).empty())
                        significant.push_back(trim(line));
                std::smatch match, define;
                if(significant.size() >= 3 &&
                   std::regex_match(significant.front(), match, ifndef_) &&
                   std::regex_match(significant[1], define, define_) &&
                   match[1].str() == define[1].str() &&
                   std::regex_match(significant.back(), endif_))
                {
                    const auto guard_name = match[1].str();
                    int depth = 0;
                    bool covers_file = true;
                    for(std::size_t i = 0; i < significant.size(); ++i)
                    {
                        if(std::regex_match(significant[i], ifndef_) ||
                           std::regex_match(significant[i], ifdef_))
                            ++depth;
                        else if(std::regex_match(significant[i], endif_))
                        {
                            --depth;
                            if(depth < 0 || (depth == 0 && i + 1 != significant.size()))
                                covers_file = false;
                        }
                    }
                    if(covers_file && depth == 0)
                        guards_[key] = guard_name;
                }

                active_.insert(key);
                dependencies.push_back(path);
                struct Conditional
                {
                    bool parent;
                    bool branch;
                    bool had_else = false;
                };
                std::vector<Conditional> conditions;
                bool enabled = true;
                std::string output;
                for(std::size_t i = 0; i < lines.size(); ++i)
                {
                    const auto text = trim(lines[i]);
                    const auto fail = [&](const std::string& code, const std::string& message)
                    {
                        throw CompilationError(code, key, i + 1, 0, message);
                    };
                    if(text.empty() || text[0] != '#')
                    {
                        output += enabled ? expand_constants(lines[i]) + "\n" : "\n";
                        continue;
                    }
                    if(std::regex_match(text, match, ifndef_) || std::regex_match(text, match, ifdef_))
                    {
                        const bool is_ifndef = std::regex_match(text, ifndef_);
                        const bool defined = macros_.count(match[1].str()) != 0;
                        const bool branch = is_ifndef ? !defined : defined;
                        conditions.push_back(Conditional{enabled, branch});
                        enabled = enabled && branch;
                    }
                    else if(std::regex_match(text, else_))
                    {
                        if(conditions.empty() || conditions.back().had_else)
                            fail("E_PP_DIRECTIVE", "unmatched or repeated #else");
                        auto& condition = conditions.back();
                        condition.had_else = true;
                        enabled = condition.parent && !condition.branch;
                    }
                    else if(std::regex_match(text, endif_))
                    {
                        if(conditions.empty())
                            fail("E_PP_DIRECTIVE", "unmatched #endif");
                        enabled = conditions.back().parent;
                        conditions.pop_back();
                    }
                    else if(!enabled)
                    {
                        // 未选中的分支不打开头文件，也不登记宏。
                    }
                    else if(std::regex_match(text, match, define_))
                    {
                        const auto name = match[1].str(), value = match[2].str();
                        const auto inserted = macros_.emplace(name, value);
                        if(!inserted.second && inserted.first->second != value)
                            fail("E_PP_DIRECTIVE", "incompatible macro redefinition: " + name);
                    }
                    else if(std::regex_match(text, pragma_once_))
                        once_.insert(key);
                    else if(std::regex_match(text, match, local_include_))
                    {
                        const auto child = path.parent_path() / match[1].str();
                        std::error_code include_error;
                        if(!fs::is_regular_file(child, include_error))
                            fail("E_PP_INCLUDE", "cannot find local header '" + match[1].str() + "'");
                        result.headers.push_back(child.lexically_normal().string());
                        output += expand(child);
                    }
                    else if(std::regex_match(text, match, system_include_))
                    {
                        if(match[1].str() == "stdio.h")
                        {
                            result.includes_stdio = true;
                            result.headers.emplace_back("<stdio.h>（受限printf声明）");
                        }
                        else if(match[1].str() == "stdbool.h")
                        {
                            // bool/true/false已由本语言直接识别，导入头文件不展开平台宏。
                            result.headers.emplace_back("<stdbool.h>（bool与true/false契约）");
                        }
                        else
                            fail("E_PP_HEADER", "unsupported system header <" + match[1].str() + ">");
                    }
                    else
                        fail("E_PP_DIRECTIVE", "unsupported preprocessing directive: " + text);
                    output += '\n';
                }
                if(!conditions.empty())
                    throw CompilationError("E_PP_DIRECTIVE", key, lines.size(), 0, "unterminated #ifdef/#ifndef");
                active_.erase(key);
                return output;
            }

        private:
            // 仅展开无参数的非负十进制整数常量；不把宏名替换进字符串。
            std::string expand_constants(const std::string& line) const
            {
                std::string result;
                char quote = 0;
                bool escaped = false;
                for(std::size_t i = 0; i < line.size();)
                {
                    const char c = line[i];
                    if(quote)
                    {
                        result += c;
                        ++i;
                        if(escaped)
                            escaped = false;
                        else if(c == '\\')
                            escaped = true;
                        else if(c == quote)
                            quote = 0;
                    }
                    else if(c == '"' || c == '\'')
                    {
                        quote = c;
                        result += c;
                        ++i;
                    }
                    else if(std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                    {
                        const auto start = i++;
                        while(i < line.size() &&
                              (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_'))
                            ++i;
                        const auto word = line.substr(start, i - start);
                        const auto macro = macros_.find(word);
                        result += macro == macros_.end() ? word : macro->second;
                    }
                    else
                    {
                        result += c;
                        ++i;
                    }
                }
                return result;
            }

            std::unordered_set<std::string> active_, once_;
            std::unordered_map<std::string, std::string> macros_;
            std::unordered_map<std::string, std::string> guards_;
            std::size_t bytes_read_ = 0;
            const std::regex ifndef_{R"(^#\s*ifndef\s+([A-Za-z_][A-Za-z_0-9]*)\s*$)"};
            const std::regex ifdef_{R"(^#\s*ifdef\s+([A-Za-z_][A-Za-z_0-9]*)\s*$)"};
            const std::regex define_{R"(^#\s*define\s+([A-Za-z_][A-Za-z_0-9]*)(?:\s+([0-9]+))?\s*$)"};
            const std::regex else_{R"(^#\s*else\s*$)"};
            const std::regex endif_{R"(^#\s*endif\s*$)"};
            const std::regex pragma_once_{R"(^#\s*pragma\s+once\s*$)"};
            const std::regex local_include_{R"inc(^#\s*include\s*"([^"\n]+)"\s*$)inc"};
            const std::regex system_include_{R"(^#\s*include\s*<([^>\n]+)>\s*$)"};
        };
    }

    Result prepare(const std::string& source, const std::string& output,
                   const std::array<std::string, 17>& generated_files)
    {
        Processor processor;
        const auto text = processor.expand(source);
        // 新增的头文件同样是输入，必须在任何中间文件被截断前检查。
        for(const auto& dependency : processor.dependencies)
        {
            for(const auto& name : generated_files)
            {
                std::error_code error;
                if(fs::weakly_canonical(fs::absolute(name)) == dependency ||
                   fs::equivalent(dependency, name, error))
                    throw CompilationError("E_PP_OUTPUT", dependency.string(), 0, 0,
                                           "generated output would overwrite an included input");
            }
        }
        std::ofstream stream(output, std::ios::binary | std::ios::trunc);
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.close();
        if(!stream)
            throw std::runtime_error("cannot write preprocessed input: " + output);
        return processor.result;
    }
}
