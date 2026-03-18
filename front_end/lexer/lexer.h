//
// Created by 86198 on 2024/10/20.
//

#ifndef MY_COMPILER_LEXER_H
#define MY_COMPILER_LEXER_H


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
            //文件操作
            input_file(file_in_, from_file);
            output_file(file_out_, to_file);

            if(file_in_.fail() || file_out_.fail())
            {
                return FAILED;
            }
            char temp, pre;
            int status=0;
            file_in_ >> std::noskipws;
            file_in_ >> temp;
            while (temp == ' ' ||  temp == '\t' || temp == '\n')
                file_in_ >> temp;
            file_out_ << temp;
            pre = temp;				//去除程序前所有的空白字符，直到第一个非换行符空格或制表符
            while (!file_in_.eof())
            {
                file_in_ >> temp;
                if (file_in_.fail())
                    break;
                if (!status) status = 1;
                switch (status)
                {
                    case 1:
                        if (temp == ' ' || temp == '\n' || temp == '\t')
                            status = 2;
                        else
                        {
                            file_out_ << temp;
                        }
                        pre = temp;
                        break;
                    case 2:
                        if (temp != ' ' && temp != '\n' && temp != '\t')
                        {
                            status = 1;
                            file_out_ << pre << temp;				//2 状态接收到一个非空白字符 则输出最近的一个空白字符
                        }
                        if (pre!='\n')
                            pre = temp;				//如换行与制表符或空格同时出现 保留换行 保证程序大体格式不被改变
                        break;
                    default:
                        cout << "An error occurred!" << endl;
                        break;						//进入default分支 说明发生比较神奇的错误
                }
            }
            close_file(file_in_, file_out_);
            return SUCCEED;
        }

        //预处理


//状态转移图如下：
//start--status:0
//status:1	read_in-ch
//		'/'		goto-2	--
//		'*'		goto-1	output-ch
//		other	goto-1	output-ch
//status:2	read_in-ch
//		'/'		goto-2	output-'/'
//		'*'		goto-3	--
//		other	goto-1	output-'/'and ch
//status:3	read_in-ch
//		'/'		goto-3	--
//		'*'		goto-4	--
//		other	goto-3	--
//status:4	read_in-ch
//		'/'		goto-5	--
//		'*'		goto-4	--
//		other	goto-3	--
//status:5	read_in-ch
//		'/'		goto-2	--
//		'*'		goto-1	output-ch
//		other	goto-1	output-ch
        Return remove_comment_aux(const string& from_file, const string& to_file)
        {
            //文件操作
            input_file(file_in_, from_file);
            output_file(file_out_, to_file);
            int status = 0;

            if(file_in_.fail() || file_out_.fail())
            {
                return FAILED;
            }

            file_in_ >> std::noskipws;

            while (file_in_ >> next_char_) {
                switch (status) {
                    case 0:
                        if (next_char_ == '/') {
                            status = 1;
                        } else {
                            file_out_ << next_char_;
                        }
                        break;

                    case 1:
                        if (next_char_ == '/') {
                            status = 2;
                        } else if (next_char_ == '*') {
                            status = 3;
                        } else {
                            file_out_ << '/';
                            file_out_ << next_char_;
                            status = 0;
                        }
                        break;

                    case 2:
                        if (next_char_ == '\n') {
                            file_out_ << next_char_;
                            status = 0;
                        }
                        break;

                    case 3:
                        if (next_char_ == '*') {
                            status = 4;
                        }
                        break;

                    case 4:
                        if (next_char_ == '/') {
                            status = 0;
                        } else {
                            status = 3;
                        }
                        break;

                    default:  // Error state
                        cout << "An error occurred!" << endl;
                        return FAILED;
                }
            }
            close_file(file_in_, file_out_);
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

            string str;
            char ch, pre;
            vector<string> strings;
            //词法单元序列
            vector<Token> tokens;
            ErrorDefines error;

            while (!file_in_.eof())
            {
                ch = file_in_.get();
                if (file_in_.fail())
                    break;
                if (is_blank_character(ch))     // 处理空白字符
                {
                    str.clear();
                }
                else if (isalpha(ch))       // 处理标识符或保留字(其第一个字符不能是数字)
                {
                    pre = ch;
                    while (isalpha(ch) || isdigit(ch) || ch == '_')
                    {
                        str.push_back(ch);
                        current_column_++;
                        ch = file_in_.get();
                    }
                    strings.push_back(str);

                    if(str == "false" || str == "true")
                    {
                        auto token = Token(TokenType::BOOLEAN, str, current_line_);
                        tokens.push_back(token);
                        file_out_ << token << "on line: " << current_line_ << "and column: " << current_column_ - str.length() << endl;
                    }
                    else if (KeywordToken::is_keyword(str))
                    {
                        auto token = KeywordToken(str, current_line_);
                        tokens.push_back(token);
                        file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - str.length() << endl;
                    }
                    else
                    {
                        auto token = IdentifierToken(str, current_line_);
                        tokens.push_back(token);
                        file_out_ << token << "on line: " << current_line_ << " and column: " << current_column_ - str.length() << endl;
                    }

                    str.clear();
                    retract(file_in_);
                }
                else if (NumberToken::is_number(ch))         // 处理数字
                {
                    pre = ch;
                    while (isdigit(ch) || ch == '.')
                    {
                        str.push_back(ch);
                        current_column_++;
                        ch = file_in_.get();
                    }
                    strings.push_back(str);
                    // 如果数字后面出现字母，报告错误
                    if (isalpha(ch))
                    {
                        cout << "[Lexical Error] [line: " << current_line_ << ", column: " << current_column_ - str.length() << "] Invalid : " << str << ch << endl;

                        while (isalpha(ch) || isdigit(ch))
                        {
                            str.push_back(ch);
                            current_column_++;
                            ch = file_in_.get();
                        }
                        //出现 illegal word 错误
                        error = ILLEGAL_WORD;
                        file_out_ << "[Lexical Error] " << errors_.find(error) + "\"" + str + "\"" << " [line: "<< current_line_ << ", column: " << current_column_ - str.length() << endl;
                    }
                    else
                    {
                        auto token = NumberToken(str, current_line_);
                        tokens.push_back(token);
                        file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - str.length() << std::endl;
                    }

                    str.clear();
                    retract(file_in_);
                }
                else if(DelimiterToken::is_delimiter(ch))
                {
                    auto token = DelimiterToken(ch, current_line_);
                    current_column_++;
                    tokens.push_back(token);
                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_  - 1<< endl;
                }
                else if(OperatorToken::is_operator(ch))
                {
                    auto token = OperatorToken(ch, current_line_);
                    pre = ch;
                    switch (ch) //其他OP类字符
                    {
                        case '=':
                            current_column_++;
                            ch = file_in_.get();
                            if (ch == '=')
                            {
                                token.set_attribute_value(string(1, pre) + ch);
                                current_column_++;
                                file_out_ << "Operator(\"" << string(1, pre) << ch << "\") on line: " << current_line_ << " and column: " << current_column_ - 2 << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = ILLEGAL_WORD;
                                    file_out_ << "[Lexical Error \"" << errors_.find(error) << "\"] " << "Expression is not assignable, line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '<':
                            current_column_++;
                            ch = file_in_.get();
                            if (ch == '=')
                            {
                                token.set_attribute_value(string(1, pre) + ch);
                                current_column_++;
                                file_out_ << "Operator(\"" << string(1, pre) + ch << "\") on line: " << current_line_ << " and column: " << current_column_ - 2 << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = EXPECTED;
                                    file_out_ << "[Lexical Error \"" << string(1, pre) << ch << "\"] " << errors_.find(error) << "expression, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '>':
                            current_column_++;
                            ch = file_in_.get();
                            if (ch == '=')
                            {
                                token.set_attribute_value(string(1, pre) + ch);
                                current_column_++;
                                file_out_ << "OperatorToken(\"" << string(1, pre) + ch << "\")" << " on line: " << current_line_ << " and column: " << current_column_ - 2 << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " " << current_line_ << " " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = EXPECTED;
                                    file_out_ << "[Lexical Error \"" << string(1, pre) + ch << "\"] " << errors_.find(error) << "expression, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '!':
                            current_column_++;
                            ch = file_in_.get();
                            if (ch == '=')
                            {
                                token.set_attribute_value(string(1, pre) + ch);
                                current_column_++;
                                file_out_ << "Operator(\"" << string(1, pre) << ch << "\") on line: " << current_line_ << " and column: " << current_column_ - 2 << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = EXPECTED;
                                    file_out_ << "[Lexical Error \"" << string(1, pre) << ch << "\"] " << errors_.find(error) << "expression, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '+':
                            current_column_++;
                            ch = file_in_.get();
                            if(ch == '=' || ch == '+')
                            {
                                current_column_++;
                                token.set_attribute_value(string(1, pre) + ch);
                                file_out_ << "Operator(\"" << string(1, pre) + ch << "\") on line: " << current_line_ - 2 << " and column: " << current_column_ << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = ILLEGAL_WORD;
                                    file_out_ << "[Lexical Error \"" << errors_.find(error) << "\"] " << "Expression result unused, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '-':
                            current_column_++;
                            ch = file_in_.get();
                            if(ch == '=' || ch == '-')
                            {
                                current_column_++;
                                token.set_attribute_value(string(1, pre) + ch);
                                file_out_ << "Operator(\"" << string(1, pre) + ch << "\") on line: " << current_line_ - 2 << " and column: " << current_column_ << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = ILLEGAL_WORD;
                                    file_out_ << "[Lexical Error \"" << errors_.find(error) << "\"] " << "Expression result unused, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        case '/':
                        case '*':
                        case '^':
                            current_column_++;
                            ch = file_in_.get();
                            if(ch == '=')
                            {
                                current_column_++;
                                token.set_attribute_value(string(1, pre) + ch);
                                file_out_ << "Operator(\"" << string(1, pre) + ch << "\") on line: " << current_line_ - 2 << " and column: " << current_column_ << endl;
                            }
                            else
                            {
                                if(isdigit(ch) || isalpha(ch) || is_blank_character(ch))
                                {
                                    file_out_ << token << " on line: " << current_line_ << " and column: " << current_column_ - 1 << endl;
                                    retract(file_in_);
                                }
                                else
                                {
                                    token.error();
                                    current_column_++;
                                    error = ILLEGAL_WORD;
                                    file_out_ << "[Lexical Error \"" << errors_.find(error) << "\"] " << "Expression result unused, on line: " << current_line_ << endl;
                                }
                            }
                            break;
                        default:	//未识别成功的信息 显示出错信息
                            current_column_++;
                            file_out_ << "[Lexical Error] " << " [" << current_line_ << "," << current_column_ - 1 << "] " << "UnKnown Expression" << ch << endl;
                    }
                    tokens.push_back(token);
                }
            }
            file_out_ << "总行数: " << current_line_ << endl;
            file_out_ << "词法分析阶段错误总数：" << errors_.total_error_number() << endl;
            close_file(file_in_, file_out_);
            return tokens;
        }


        void _remove_comment(const string& from_file, const string& to_file)
        {
            if(remove_comment_aux(from_file, to_file) == FAILED)
            {
                exit(1);
            }
            cout << "去除注释成功" << endl;
        }

        void _remove_blank(const string& from_file, const string& to_file)
        {
            if(remove_blank_aux(from_file, to_file) == FAILED)
            {
                exit(1);
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




        vector<Token> lexical_analysis()
        {
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
    };


}



#endif //MY_COMPILER_LEXER_H
