#include "driver_options.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <string>
#include <system_error>

namespace my_compiler::driver
{
    namespace fs = std::filesystem;

    void print_help(std::ostream& out)
    {
        out << "Usage: my_compiler --input SOURCE --grammar GRAMMAR --output-dir DIR\n"
            << "       [--emit legacy|ast|llvm] [--opt-level 0|2]\n"
            << "       my_compiler --help\n"
            << "With no options, use the legacy ../files layout.\n"
            << "AST: int functions, locals, assignments, braced if/else and one final return; LLVM requires an enabled build.\n"
            << "Inputs are read-only; generated artifacts are written under DIR.\n"
            << "Exit: 0=run completed, 1=detected compilation/file/runtime failure,\n"
            << "      2=invalid options. Diagnostics use stderr.\n"
            << "Source locations refer to preprocessed files; use a fresh output directory.\n";
    }

    Options parse_options(int argc, char* argv[])
    {
        Options options;
        if(argc == 1)
            return options;
        std::map<std::string, std::string> values;
        for(int i = 1; i < argc; ++i)
        {
            const std::string name = argv[i];
            if(name == "--help" || name == "-h")
            {
                options.help = true;
                return options;
            }
            if(name != "--input" && name != "--grammar" && name != "--output-dir" &&
               name != "--emit" && name != "--opt-level")
                throw UsageError("unknown option: " + name);
            if(++i == argc || std::string(argv[i]).empty() ||
               std::string(argv[i]).rfind("--", 0) == 0)
                throw UsageError("missing value for " + name);
            if(!values.emplace(name, argv[i]).second)
                throw UsageError("duplicate option: " + name);
        }
        for(const char* required : {"--input", "--grammar", "--output-dir"})
            if(values.find(required) == values.end())
                throw UsageError(std::string("required option: ") + required);
        const auto mode = values.find("--emit");
        if(mode != values.end())
        {
            if(mode->second == "legacy")
                options.emit = EmitKind::Legacy;
            else if(mode->second == "ast")
                options.emit = EmitKind::Ast;
            else if(mode->second == "llvm")
                options.emit = EmitKind::LLVM;
            else
                throw UsageError("--emit must be legacy, ast or llvm");
        }
        const auto level = values.find("--opt-level");
        if(level != values.end())
        {
            if(options.emit != EmitKind::LLVM)
                throw UsageError("--opt-level requires --emit llvm");
            if(level->second == "0")
                options.optimization_level = 0;
            else if(level->second == "2")
                options.optimization_level = 2;
            else
                throw UsageError("--opt-level must be 0 or 2");
        }
        options.paths = CompilationPaths::for_run(
            fs::absolute(values.at("--input")).lexically_normal(),
            fs::absolute(values.at("--grammar")).lexically_normal(),
            fs::absolute(values.at("--output-dir")).lexically_normal());
        return options;
    }

    namespace
    {
        void require_readable_file(const fs::path& path, const char* kind)
        {
            std::error_code ec;
            if(!fs::is_regular_file(path, ec))
                throw std::runtime_error(std::string(kind) + " is not a regular file: " + path.string());
            std::ifstream stream(path, std::ios::binary);
            if(!stream)
                throw std::runtime_error(std::string("cannot read ") + kind + ": " + path.string());
        }

        bool same_file(const fs::path& a, const fs::path& b)
        {
            std::error_code ec;
            const bool equal = fs::equivalent(a, b, ec);
            return !ec && equal;
        }
    } // namespace

    void validate_and_prepare(const CompilationPaths& paths)
    {
        require_readable_file(paths.source_file, "source");
        require_readable_file(paths.grammar_file, "grammar");
        const auto source = fs::canonical(paths.source_file);
        const auto grammar = fs::canonical(paths.grammar_file);
        // 打开或截断任何输出文件前，先验证全部目标路径，避免覆盖输入。
        for(const auto& name : paths.output_files())
        {
            const auto output = fs::weakly_canonical(fs::absolute(name));
            if(output == source || output == grammar ||
               same_file(output, source) || same_file(output, grammar))
                throw UsageError("output would overwrite an input file: " + name);
        }
        for(const auto& name : paths.output_files())
            fs::create_directories(fs::path(name).parent_path());
    }
} // namespace my_compiler::driver
