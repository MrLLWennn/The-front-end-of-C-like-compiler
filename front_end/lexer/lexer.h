//
// 创建者：86198；日期：2024/10/20。
//

#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H

#include "my_compiler/compilation_error.hpp"
#include "my_compiler/preprocessor.hpp"
#include <cctype>
#include <regex>


#include"../../token/token_header.h"
#include"state_transition/state_transition.h"
#include"../../ErrorHandler/ErrorTable.h"
#include"../parser/parser.h"


//------------------------------------------------------------------------------


/*
 * ————预处理
 *  1） 去除注释
 *  2） 去除多余的空白：空格，\t，\n
 *  * 3)  将语法根据 ‘|’ 拆分
 * */


//删除程序中的注释
//参数一：待处理的文件
//参数二：去除注释后的文件






//------------------------------------------------------------------------------


/*
 *
 * ---- 词法分析器
 *
 * 1.主要任务————在编译过程负责读取源程序的输入字符，将它们变成词素，生成并输出一个词法单元序列，每个词法单元对应于一个词素
 * 2.还会完成一些识别词素只玩的其他任务：
 *      1）过滤掉源程序中的注释和空白（空格，换行符，制表符以及在输入中用于分隔词法单元的其他字符）
 *      2）编译器生成的错误消息与源程序的位置联系起来（如：负责记录遇到的换行符个数，以便给每个错误消息赋予一个行号）
 * 3.可以将其分为两个级联的阶段：
 *      1）扫描阶段：负责完成一些不需要生成词法单元的简单处理，去除注释空白
 *      2）词法分析阶段：处理扫描阶段的输出并生成词法单元
 * 4.将词法分析和语法分析划分的意义
 *      1）简化编译器设计。可以至少简化其中一项设计，去除过多的耦合度
 *      2）提高编译器的效率。将专用于本身只属于词法分析阶段处理的任务独立出来
 *      3）增强编译器的可移植性。将输入设备的特殊性限制在词法分析器中
 * 5.专有术语
 *      1）词法单元：有一个词法单元名和一个可选的属性值组成。使用词法单元名来引用一个词法单元。
 *      2）模式：描述了一个此法单元的词素可能具有的形式
 *      3）词素：是源程序中的一个字符序列，和某个词法单元的模式匹配，并被词法分析器识别为该词法单元的一个实例
 * 6.设计
 *   ————词法单元
 *      1）每个关键字有一个词法单元。一个关键字的模式就是该关键字本身
 *      2）表示运算符的词法单元
 *      3）一个表示所有标识符的词法单元
 *      4）一个或多个表示常量的词法单元，比如数字和字面量值字符串
 *      5）每一个标点符号有一个词法单元，比如左 右括号，逗号和分号
 *
 *  7.优化
 *    分析可知，还可以加快源程序读入速度，故需要进行优化，具体方法：
 *      ————缓冲区对
 *      * 由于在编译一个大型源程序时需要处理大量的字符，处理这些字符需要很多的时间，一种机制————利用两个交替读入的缓冲区
 *          1）每个缓冲区对的容量都是N个字符（通常是一个磁盘块的大小，如果不足N个会有一个特殊的标记符号（eof））
 *          2) lexemeBegin指针：该指针指向词素的开始处。当前我们正在试图确定这个词素的结尾。
 *          3）forward指针：它一直向前扫描，直到发现某个模式被匹配为止。
 *          4）一旦确定下一个词素，forward指针将指向词素结尾的字符。在处理完这个词素后，将会被左移一个位置。
 *          5）将forward 指针前移要求我们首先检查是否已经到达某个缓冲区的末尾。若是，需要加载另一个缓冲区，且将forward指针指向这个新缓冲区对的头部
 *          6）增加哨兵结点：在末尾增加一个哨兵结点eof，就可以把对缓冲区是否为末端的检验和对当前字符的确定检验（原本为两个工作）合二为一。
 * */

namespace my_compiler
{

    using namespace my_compiler_token;
    using namespace RE_NFA_DFA;
    using namespace my_compiler_error_handler;
    class Parser;

    //词法分析器
    class Lexer
    {
        friend class Parser;
    public:

        Lexer()
        :next_char_(), current_line_(1), current_column_(1), errors_(current_line_)
        {
        }



        ~Lexer()
        {
            file_in_.close();
            file_out_.close();
            cout << "输入输出文件均已关闭" << endl;
        }

    private:

        Return remove_blank_aux(const string& from_file, const string& to_file)
        {
            input_file(file_in_, from_file);
            output_file(file_out_, to_file);
            if(file_in_.fail() || file_out_.fail())
                return FAILED;
            char ch = 0, quote = 0;
            bool escaped = false, has_content = false, pending_space = false, pending_line = false;
            while(file_in_.get(ch))
            {
                if(quote)
                {
                    // 字符串内部逐字保留，不能折叠空格或制表符。
                    file_out_.put(ch);
                    if(escaped)
                        escaped = false;
                    else if(ch == '\\')
                        escaped = true;
                    else if(ch == quote)
                        quote = 0;
                    continue;
                }
                if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                {
                    pending_space = true;
                    pending_line = pending_line || ch == '\n';
                    continue;
                }
                if(has_content && pending_space)
                    file_out_.put(pending_line ? '\n' : ' ');
                pending_space = pending_line = false;
                has_content = true;
                file_out_.put(ch);
                if(ch == '"' || ch == '\'')
                    quote = ch;
            }
            if(file_in_.bad())
                throw std::runtime_error("Cannot read input file: " + from_file);
            close_file(file_in_, file_out_);
            return SUCCEED;
        }

        //预处理


        // 先展开本地头文件并去注释；stdio.h提供受限printf声明契约。
        Return remove_comment_aux(const string& from_file, const string& to_file)
        {
            const auto result = preprocessing::prepare(
                from_file, to_file, legacy_compilation_paths().output_files());
            includes_stdio_ = result.includes_stdio;
            for(const auto& header : result.headers)
                cout << "导入头文件：" << header << endl;
            return SUCCEED;
        }


        //空白字符的行列 变化
        bool is_blank_character(const char& ch)
        {
            switch(ch)
            {
                case ' ':
                    current_column_++;
                    break;
                case '\t':
                    current_column_ += 4;
                    break;
                case '\r':
                case '\n':
                    current_column_ = 1;
                    current_line_++;
                    break;
                default:
                    return false;
            }
            return true;
        }


        //对所读取的文件回退一个字符
        static void retract(fstream& file)
        {
            if (file.tellg() > 1) // 确保不在文件开头
            {
                file.unget(); // 将文件流的当前位置移到前一个字符
            }
            else  // 文件指针在开头，不能回退
            {
                file.seekg(0);  // 确保文件指针处于有效位置
            }
        }


        vector<Token> tokenize_aux(const string& from_file, const string& to_file)
        {
            input_file(file_in_, from_file);
            output_file(file_out_, to_file);
            vector<Token> tokens;
            char ch = 0;
            const auto emit = [&](TokenType kind, const string& value, size_t line, size_t column)
            {
                Token token(kind, value, line);
                tokens.push_back(token);
                file_out_ << token << " on line: " << line << " and column: " << column << endl;
            };
            // 整数、带小数点或指数的浮点数；f/F后缀只属于浮点字面量。
            const std::regex number_pattern(
                R"((?:[0-9]+|(?:(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:[eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+)[fF]?))");
            while(file_in_.get(ch))
            {
                if(is_blank_character(ch))
                    continue;
                const auto token_line = current_line_;
                const auto token_column = current_column_++;
                if(ch == '"' || ch == '\'')
                {
                    const char quote = ch;
                    const bool character = quote == '\'';
                    const string code = character ? "E_LEX_CHARACTER" : "E_LEX_STRING";
                    string literal(1, quote);
                    bool closed = false;
                    size_t units = 0;
                    while(file_in_.get(ch))
                    {
                        ++current_column_;
                        if(ch == '\n' || ch == '\r' || ch == '\0')
                            throw CompilationError(code, from_file, token_line, token_column,
                                character ? "newline or NUL in character literal" :
                                "newline or NUL in string literal");
                        literal += ch;
                        if(ch == quote)
                        {
                            closed = true;
                            break;
                        }
                        if(ch == '\\')
                        {
                            if(!file_in_.get(ch))
                                break;
                            ++current_column_;
                            if(ch != 'n' && ch != 't' && ch != 'r' && ch != '\\' && ch != '"' &&
                               ch != '\'' && !(character && ch == '0'))
                                throw CompilationError(code, from_file, token_line, token_column,
                                    character ? "unsupported character escape" : "unsupported string escape");
                            literal += ch;
                        }
                        else if(character && static_cast<unsigned char>(ch) > 127)
                            throw CompilationError(code, from_file, token_line, token_column,
                                "character literals support ASCII bytes only");
                        ++units;
                    }
                    if(!closed)
                        throw CompilationError(code, from_file, token_line, token_column,
                            character ? "unterminated character literal" : "unterminated string literal");
                    if(character && units != 1)
                        throw CompilationError(code, from_file, token_line, token_column,
                            "character literal requires exactly one character");
                    emit(character ? TokenType::CHARACTER : TokenType::STRING, literal, token_line, token_column);
                }
                else if(std::isalpha(static_cast<unsigned char>(ch)) || ch == '_')
                {
                    string value(1, ch);
                    while(file_in_.peek() != EOF &&
                          (std::isalnum(static_cast<unsigned char>(file_in_.peek())) || file_in_.peek() == '_'))
                    {
                        value += static_cast<char>(file_in_.get());
                        ++current_column_;
                    }
                    const auto kind = value == "true" || value == "false" ? TokenType::BOOLEAN :
                        KeywordToken::is_keyword(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
                    emit(kind, value, token_line, token_column);
                }
                else if(std::isdigit(static_cast<unsigned char>(ch)) ||
                        (ch == '.' && file_in_.peek() != EOF && std::isdigit(static_cast<unsigned char>(file_in_.peek()))))
                {
                    string value(1, ch);
                    while(file_in_.peek() != EOF)
                    {
                        const auto next = static_cast<char>(file_in_.peek());
                        const bool exponent_sign = (next == '+' || next == '-') &&
                            (value.back() == 'e' || value.back() == 'E');
                        if(!std::isalnum(static_cast<unsigned char>(next)) && next != '.' && !exponent_sign)
                            break;
                        value += static_cast<char>(file_in_.get());
                        ++current_column_;
                    }
                    if(!std::regex_match(value, number_pattern))
                        throw CompilationError("E_LEX_NUMBER", from_file, token_line, token_column,
                            "invalid numeric literal '" + value + "'");
                    emit(TokenType::NUMBER, value, token_line, token_column);
                }
                else if(DelimiterToken::is_delimiter(ch))
                    emit(TokenType::DELIMITER, string(1, ch), token_line, token_column);
                else if(OperatorToken::is_operator(ch))
                {
                    string value(1, ch);
                    const int next = file_in_.peek();
                    if(next == '=' || (ch == '+' && next == '+') || (ch == '-' && (next == '-' || next == '>')))
                    {
                        value += static_cast<char>(file_in_.get());
                        ++current_column_;
                    }
                    emit(TokenType::OPERATOR, value, token_line, token_column);
                }
                else
                    throw CompilationError("E_LEX_CHAR", from_file, token_line, token_column,
                        "unsupported character byte " + std::to_string(static_cast<unsigned char>(ch)));
            }
            if(file_in_.bad())
                throw std::runtime_error("Cannot read input file: " + from_file);
            file_out_ << "总行数: " << current_line_ << endl;
            file_out_ << "词法分析阶段错误总数：0" << endl;
            close_file(file_in_, file_out_);
            return tokens;
        }

        void _remove_comment(const string& from_file, const string& to_file)
        {
            if(remove_comment_aux(from_file, to_file) == FAILED)
            {
                throw std::runtime_error("preprocessing failed");
            }
            cout << "去除注释成功" << endl;
        }

        void _remove_blank(const string& from_file, const string& to_file)
        {
            if(remove_blank_aux(from_file, to_file) == FAILED)
            {
                throw std::runtime_error("preprocessing failed");
            }
            cout << "去除空白成功" << endl;
        }

    public:

        void remove_comment()
        {
            //从 源文件 读入，结果输出到 去除注释 文件中
            _remove_comment(source_file, uncomment_file);
        }


        void remove_blank()
        {
            //从 去除注释 的文件读入， 结果输出到 预处理 文件中
            _remove_blank(uncomment_file, pretreatment_file);
        }


        vector<Token> tokenize()
        {
            vector<Token> tokens = tokenize_aux(pretreatment_file, lex_file);
            return tokens;
        }




        bool includes_stdio() const noexcept
        {
            return includes_stdio_;
        }

        vector<Token> lexical_analysis()
        {
            current_line_ = current_column_ = 1;
            includes_stdio_ = false;
            //去除注释
            cout << "************------------ 过程：" << period[1] << "----步骤：" << step[1] << "------------************" << endl;
            cout << "文件" << source_file << "去除注释..." << endl;
            cout << "文件写入" << uncomment_file << endl;
            remove_comment();
            divide_line();
            //去除空白
            cout << "************------------ 过程：" << period[1] << "----步骤：" << step[2] << "------------************" << endl;
            cout << "文件" << uncomment_file << "去除空白" << endl;
            cout << "文件写入" << pretreatment_file << endl;
            remove_blank();
            divide_line();
            //生成词法单元序列
            cout << "************------------ 过程：" << period[2] << "----步骤：" << step[3] << "------------************" << endl;
            cout << "文件" << pretreatment_file << "生成词法单元序列" << endl;
            cout << "文件写入" << lex_file << endl;
            return tokenize();
        }


    private:
        //下一个读入的字符
        char next_char_;
        //文件流对象
        fstream file_in_;
        fstream file_out_;
        //当前行数
        size_t current_line_;
        //当前列数
        size_t current_column_;
        ErrorTable errors_;
        bool includes_stdio_ = false;
    };


}



#endif //MY_COMPILER_LEXER_H
