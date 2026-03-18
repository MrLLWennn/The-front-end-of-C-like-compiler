//
// Created by 86198 on 2024/11/4.
//

#include "front_end.h"



void my_compiler::FrontEnd::_front_end_compiling_aux()
{

    tokens_ = lexer_.lexical_analysis();
    //将上述词法分析器得到的vector<Token> 用来 parser 使用

    parser_.grammatical_analysis(tokens_);
}


void my_compiler::FrontEnd::front_end_compiling_aux()
{
    _front_end_compiling_aux();

    //生成符号表
    cout << "************------------ 过程：" << period[4] << "----步骤：" << step[11] << "------------************" << endl;
    cout << "读入文件" << lex_file << endl;
    cout << "文件写入" << symbol_table_file << endl;
    generate_symbol_table();
    divide_line();


    //进行语义分析
    cout << "************------------ 过程：" << period[4] << "----步骤：" << step[12] << "------------************" << endl;
    cout << "读入文件" << processed_grammar_file << endl;
    cout << "文件写入" << semantic_analysis_file << endl;
    divide_line();
}




void my_compiler::FrontEnd::generate_symbol_table_aux()
{

    string temp;

    struct temp_token
    {
        size_t line_;
        string value_;
        string type_;
        temp_token(): line_(0){}
    };

    vector<temp_token> tokens;
    tokens.resize(tokens_.size());
    for(size_t i = 0;i < tokens_.size();i++)
    {
        tokens[i].value_ = tokens_[i].get_attribute_value();
        tokens[i].line_ = tokens_[i].get_line();
        tokens[i].type_ = get_token_name(tokens_[i].get_token_name_());
    }
    vector<string> file_lines;
    //读取文件后两行之前的
    while(std::getline(file_in_, temp))
    {
        if(file_in_.fail())
        {
            cout << "读入失败" << endl;
            break;
        }
        file_lines.emplace_back(temp);
    }
    file_lines.pop_back();
    file_lines.pop_back();



    // 更改tokens中的 Identifier
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].type_ == "Identifier")
        {
            // 检查 i + 1 是否越界
            if (i + 1 < tokens.size() && tokens[i + 1].value_ == "(")
            {
                // 标识符是函数
                tokens[i].type_ = "identifier-function";
            }
            else if (i > 0 && tokens[i - 1].value_ == "struct")
            {
                // 标识符是结构体
                tokens[i].type_ = "identifier-struct";
            }
            else if (i + 1 < tokens.size() &&
                     (tokens[i + 1].value_ == "=" ||
                      tokens[i + 1].value_ == "+=" ||
                      tokens[i + 1].value_ == "-=" ||
                      tokens[i + 1].value_ == "/=" ||
                      tokens[i + 1].value_ == "*=")) {
                // 标识符是变量赋值
                tokens[i].type_ = "identifier1";
            }
            else
            {
                // 其他标识符
                std::transform(tokens[i].type_.begin(), tokens[i].type_.end(), tokens[i].type_.begin(), ::tolower);
            }
        }
        else
        {
            // 直接处理不为 Identifier 的 token
            std::transform(tokens[i].type_.begin(), tokens[i].type_.end(), tokens[i].type_.begin(), ::tolower);
        }
    }


    //表征在此之前出现过 左大括号 { 则为0， 否则 > 0
    int left_brace = 0;

    for(size_t i = 0;i < tokens.size();)
    {
        if(tokens[i].type_ == "identifier-function")
        {
            //不存在多余的左大括号
            if(left_brace == 0)
            {
                //返回值不为 const 的函数
                if(i == 1 || (i >= 2 && tokens[i].value_ != "const"))
                {
                    string type = tokens[i - 1].value_;
                    string name = tokens[i].value_;
                    FunctionSymbol func(Category::FUNC, name, type);
                    func.set_address(&func);
                    func.set_property("global function");
                    //将参数加入 parameters_中
                    i += 2;
                    for(size_t j = i;j < tokens.size();j++)
                    {
                        if(tokens[j].value_ == ")")
                        {
                            i = j + 1;
                            break;
                        }

                        if(tokens[j].type_ == "identifier")
                        {
                            if(tokens[j - 2].value_ == "const")
                            {
                                string type1 = tokens[j - 2].value_.append(" ").append(tokens[j - 1].value_);
                                string name1 = tokens[j].value_;
                                if(tokens[j + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function parameter");
                                    func.emplace_parameter(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function array parameter");
                                    func.emplace_parameter(sym);
                                }
                            }
                            else
                            {
                                string type1 = tokens[j  - 1].value_;
                                string name1 = tokens[j].value_;
                                if(tokens[j + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function parameter");
                                    func.emplace_parameter(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function array parameter");
                                    func.emplace_parameter(sym);
                                }
                            }
                        }
                    }

                    //接下来处理该函数体
                    symbol_table_.emplace_function(func);
                    int inner_brace = 1;
                    if(inner_brace != 0)
                    {
                        for(size_t k = i + 1;k < tokens.size();)
                        {
                            //即为函数局部变量声明
                            if((tokens[k - 1].value_ == "int"   ||
                                tokens[k - 1].value_ == "void"  ||
                                tokens[k - 1].value_ == "float" ||
                                tokens[k - 1].value_ == "double"||
                                tokens[k - 1].value_ == "bool") &&
                               (tokens[k].type_ == "identifier" || tokens[k].type_ == "identifier1"))
                            {
                                //处理 for 循环中的变量声明
                                if(k >= 3 && tokens[k - 3].value_ == "for")
                                {
                                    string type1 = tokens[k - 1].value_;
                                    string name1 = tokens[k].value_;
                                    Symbol sym(Category::VAR, name1, type1, inner_brace, false);
                                    sym.set_address(&sym);
                                    sym.set_property("for circulation iteration symbol");
                                    symbol_table_.emplace_circulation_symbol(func.get_function_name(), sym);
                                    k++;
                                }
                                else
                                {
                                    if(tokens[k - 2].value_ == "const")
                                    {
                                        string type1 = tokens[k - 2].value_.append(" ").append(tokens[k - 1].value_);
                                        string name1 = tokens[k].value_;
                                        if(tokens[k + 1].value_ == "[")
                                        {
                                            Symbol sym(Category::CONST_ARRAY, name1, type1, inner_brace, false);
                                            sym.set_address(&sym);
                                            sym.set_property("global function local array symbol");
                                            symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                        }
                                        else
                                        {
                                            Symbol sym(Category::CONST_VAR, name1, type1, inner_brace, false);
                                            sym.set_address(&sym);
                                            sym.set_property("global function local symbol");
                                            symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                        }
                                    }
                                    else
                                    {
                                        string type1 = tokens[k - 1].value_;
                                        string name1 = tokens[k].value_;
                                        if(tokens[k + 1].value_ == "[")
                                        {
                                            Symbol sym(Category::ARRAY, name1, type1, inner_brace, false);
                                            sym.set_address(&sym);
                                            sym.set_property("global function local array symbol");
                                            symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                        }
                                        else
                                        {
                                            Symbol sym(Category::VAR, name1, type1, inner_brace, false);
                                            sym.set_address(&sym);
                                            sym.set_property("global function local symbol");
                                            symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                        }
                                    }
                                    k++;
                                }

                            }
                            else if(tokens[k].type_ == "identifier" || tokens[k].type_ == "identifier1")    //变量使用
                            {
                                if(symbol_table_.global_symbol_exists(tokens[k].value_))
                                {
                                    cout << "使用的变量为全局变量" << endl;
                                }
                                else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[k].value_))
                                {
                                    cout << "函数内局部变量" << endl;
                                }
                                else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[k].value_))
                                {
                                    cout << "函数参数" << endl;
                                }
                                else if(symbol_table_.for_symbol_exists(func.get_function_name(), tokens[k].value_))
                                {
                                    cout << "for 循环迭代变量" << endl;
                                }
                                k++;
                            }
                            else if(tokens[k].type_ == "identifier-function")   //函数体内出现函数，此时的函数即为函数调用
                            {
                                symbol_table_.get_function(name).emplace_function_call(tokens[k].value_);
                                //需要观察 符号表中是否存在该函数的符号项
                                if(symbol_table_.function_exists(tokens[k].value_)) //如果存在
                                {

                                    if(tokens[k + 1].value_ != "(")
                                    {
                                        //error handler
                                        cout << "焯" << endl;
                                        exit(1);
                                    }
                                    else if(tokens[k].value_ == "output")   //特殊内置函数，规定传入参数为1
                                    {
                                        for(size_t j = k + 2;j < tokens.size();)
                                        {
                                            //如果 output 函数 传入参数为另一个函数的结果
                                            if(tokens[j].type_ == "identifier-function")
                                            {
                                                if(symbol_table_.function_exists(tokens[j].value_))
                                                {
                                                    auto cur_func = symbol_table_.get_function(tokens[j].value_);
                                                    size_t count = 0;
                                                    j += 2;
                                                    for(size_t m = j;m < tokens.size();m++)
                                                    {
                                                        if(tokens[m].value_ == ")")
                                                        {
                                                            j = m + 1;
                                                            break;
                                                        }

                                                        //如果是参数
                                                        if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                        {
                                                            if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                            {
                                                                cout << "调用时参数为全局变量" << endl;
                                                            }
                                                            else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                            {
                                                                cout << "调用时参数为局部变量" << endl;
                                                            }
                                                            else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[j].value_))
                                                            {
                                                                cout << "调用时参数为定义时的参数" << endl;
                                                            }
                                                            else
                                                            {
                                                                cout << "undefined identifier" << endl;
                                                                throw std::exception();
                                                            }
                                                        }
                                                        else if(tokens[m].value_ == ",")
                                                        {
                                                            count++;
                                                        }
                                                    }
                                                    if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                    {
                                                        cout << "参数正确" << endl;
                                                    }
                                                    k = j;
                                                }
                                                break;
                                            }
                                            else if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")    //如果 output 函数的传入参数为 普通变量
                                            {
                                                size_t count = 0;
                                                for(size_t m = j + 1;m < tokens.size();)
                                                {
                                                    if(tokens[m].value_ == ")")
                                                    {
                                                        j = m + 1;
                                                        break;
                                                    }

                                                    //如果是参数
                                                    if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                    {
                                                        if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                        {
                                                            cout << "调用时参数为全局变量" << endl;
                                                        }
                                                        else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                        {
                                                            cout << "调用时参数为局部变量" << endl;
                                                        }
                                                        else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[j].value_))
                                                        {
                                                            cout << "调用时参数为定义时的参数" << endl;
                                                        }
                                                        else if(symbol_table_.for_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                        {
                                                            cout << "调用参数为for循环迭代变量" << endl;
                                                        }
                                                        else
                                                        {
                                                            cout << "undefined identifier" << endl;
                                                            throw std::exception();
                                                        }
                                                    }
                                                    else if(tokens[m].value_ == ",")
                                                    {
                                                        count++;
                                                    }
                                                }
                                                if(count == (func.parameters_size() - 1) || (count == 0 && func.parameters_size() == 0))
                                                {
                                                    cout << "参数正确" << endl;
                                                }
                                                k = j;
                                                break;
                                            }
                                        }

                                    }
                                    else
                                    {
                                        FunctionSymbol& cur_func = symbol_table_.get_function(tokens[k].value_);
                                        k+=2;
                                        size_t count = 0;
                                        //查看参数列表是否正确
                                        for(size_t j = k; j < tokens.size();j++)
                                        {
                                            if(tokens[j].value_ == ")")
                                            {
                                                k = j + 1;
                                                break;
                                            }
                                            //如果是参数
                                            if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                            {
                                                if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                {
                                                    cout << "调用时参数为全局变量" << endl;
                                                }
                                                else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                {
                                                    cout << "调用时参数为局部变量" << endl;
                                                }
                                                else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[j].value_))
                                                {
                                                    cout << "调用时参数为定义时的参数" << endl;
                                                }
                                                else
                                                {
                                                    cout << "undefined identifier" << endl;
                                                    throw std::exception();
                                                }
                                            }
                                            else if(tokens[j].value_ == ",")
                                            {
                                                count++;
                                            }
                                        }
                                        if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                        {
                                            cout << "参数正确" << endl;
                                        }
                                        else
                                        {
                                            cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                            throw std::exception();
                                        }
                                    }
                                }
                            }
                            else if(tokens[k].value_ == "{")
                            {
                                inner_brace++;
                                k++;
                            }
                            else if(tokens[k].value_ == "}")
                            {
                                inner_brace--;
                                k++;
                                if(inner_brace == 0)
                                {
                                    i = k + 1;
                                    break;
                                }
                            }
                            else
                            {
                                k++;
                            }
                        }
                    }
                }
                else if(i - 2 >= 0 && tokens[i - 2].value_ == "const")
                {

                    //即为函数签名
                    //获取该函数的type-specifier以及形参
                    //函数返回类型为常值
                    string type = tokens[i - 2].value_.append(" ").append(tokens[i - 1].value_);
                    string name = tokens[i].value_;
                    FunctionSymbol func(Category::CONST_FUNC, name, type);
                    func.set_address(&func);
                    func.set_property("global function");
                    //将参数加入 parameters_中
                    i += 2;
                    for(size_t j = i;j < tokens.size();j++)
                    {
                        if(tokens[j].value_ == ")")
                        {
                            i = j + 1;
                            break;
                        }

                        if(tokens[j].type_ == "identifier")
                        {
                            if(tokens[j - 2].value_ == "const")
                            {
                                string type1 = tokens[j - 2].value_.append(" ").append(tokens[j - 1].value_);
                                string name1 = tokens[j].value_;
                                if(tokens[j + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function parameter");
                                    func.emplace_parameter(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function array parameter");
                                    func.emplace_parameter(sym);
                                }
                            }
                            else
                            {
                                string type1 = tokens[j  - 1].value_;
                                string name1 = tokens[j].value_;
                                if(tokens[j + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function parameter");
                                    func.emplace_parameter(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                    sym.set_address(&sym);
                                    sym.set_property("global function array parameter");
                                    func.emplace_parameter(sym);
                                }
                            }
                        }
                    }
                    //接下来处理该函数体
                    symbol_table_.emplace_function(func);
                    int inner_brace = 1;
                    if(inner_brace != 0)
                    {
                        for(size_t k = i + 1;k < tokens.size();)
                        {
                            //即为函数局部变量声明
                            if((tokens[k - 1].value_ == "int"   ||
                                tokens[k - 1].value_ == "void"  ||
                                tokens[k - 1].value_ == "float" ||
                                tokens[k - 1].value_ == "double"||
                                tokens[k - 1].value_ == "bool") &&
                               tokens[k].type_ == "identifier")
                            {
                                if(tokens[k - 2].value_ == "const")
                                {
                                    string type1 = tokens[k - 2].value_.append(" ").append(tokens[k - 1].value_);
                                    string name1 = tokens[k].value_;
                                    if(tokens[k + 1].value_ == "[")
                                    {
                                        Symbol sym(Category::CONST_ARRAY, name1, type1, inner_brace, false);
                                        sym.set_address(&sym);
                                        sym.set_property("global function array parameter");
                                        symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                    }
                                    else
                                    {
                                        Symbol sym(Category::CONST_VAR, name1, type1, inner_brace, false);
                                        sym.set_address(&sym);
                                        sym.set_property("global function parameter");
                                        symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                    }
                                }
                                else
                                {
                                    string type1 = tokens[k - 1].value_;
                                    string name1 = tokens[k].value_;
                                    if(tokens[k + 1].value_ == "[")
                                    {
                                        Symbol sym(Category::CONST_ARRAY, name1, type1, inner_brace, false);
                                        sym.set_address(&sym);
                                        sym.set_property("global function local array symbol");
                                        symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                    }
                                    else
                                    {
                                        Symbol sym(Category::CONST_VAR, name1, type1, inner_brace, false);
                                        sym.set_address(&sym);
                                        sym.set_property("global function local symbol");
                                        symbol_table_.get_function(func.get_function_name()).emplace_local_symbol(sym);
                                    }
                                }
                                k++;
                            }
                            else if(tokens[k].type_ == "identifier" || tokens[k].type_ == "identifier1")    //变量使用
                            {
                                if(symbol_table_.global_symbol_exists(tokens[k].value_))
                                {
                                    cout << "使用的变量为全局变量" << endl;
                                }
                                else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[k].value_))
                                {
                                    cout << "函数内局部变量" << endl;
                                }
                                else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[k].value_))
                                {
                                    cout << "函数参数" << endl;
                                }
                                k++;
                            }
                            else if(tokens[k].type_ == "identifier-function")   //函数体内出现函数，此时的函数即为函数调用
                            {
                                cout << name << endl;
                                symbol_table_.get_function(name).emplace_function_call(tokens[k].value_);
                                //需要观察 符号表中是否存在该函数的符号项
                                if(symbol_table_.function_exists(tokens[k].value_)) //如果存在
                                {

                                    if(tokens[k + 1].value_ != "(")
                                    {
                                        //error handler
                                        cout << "焯" << endl;
                                        exit(1);
                                    }
                                    else if(tokens[k].value_ == "output")
                                    {

                                        for(size_t j = k + 2;j < tokens.size();)
                                        {
                                            if(tokens[j].type_ == "identifier-function")
                                            {
                                                if(symbol_table_.function_exists(tokens[j].value_))
                                                {
                                                    auto cur_func = symbol_table_.get_function(tokens[j].value_);
                                                    size_t count = 0;
                                                    j += 2;
                                                    for(size_t m = j;m < tokens.size();m++)
                                                    {
                                                        if(tokens[m].value_ == ")")
                                                        {
                                                            j = m + 1;
                                                            break;
                                                        }

                                                        //如果是参数
                                                        if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                        {
                                                            if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                            {
                                                                cout << "调用时参数为全局变量" << endl;
                                                            }
                                                            else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                            {
                                                                cout << "调用时参数为局部变量" << endl;
                                                            }
                                                            else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[j].value_))
                                                            {
                                                                cout << "调用时参数为定义时的参数" << endl;
                                                            }
                                                        }
                                                        else if(tokens[m].value_ == ",")
                                                        {
                                                            count++;
                                                        }
                                                    }
                                                    if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                    {
                                                        cout << "参数正确" << endl;
                                                    }
                                                    else
                                                    {
                                                        cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                        throw std::exception();
                                                    }
                                                    k = j;
                                                }
                                                break;
                                            }
                                            else if(tokens[j].type_ == "identifier")
                                            {
                                                for(size_t m = j;m < tokens.size();m++)
                                                {
                                                    if(tokens[j].value_ == ",")
                                                    {
                                                        cout << "参数过多" << endl;
                                                        exit(1);
                                                    }
                                                }

                                                break;
                                            }
                                            else
                                            {
                                                j++;
                                            }
                                        }

                                    }
                                    else
                                    {
                                        FunctionSymbol& cur_func = symbol_table_.get_function(tokens[k].value_);
                                        k+=2;
                                        size_t count = 0;
                                        //查看参数列表是否正确
                                        for(size_t j = k; j < tokens.size();j++)
                                        {
                                            if(tokens[j].value_ == ")")
                                            {
                                                k = j + 1;
                                                break;
                                            }
                                            //如果是参数
                                            if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                            {
                                                if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                {
                                                    cout << "调用时参数为全局变量" << endl;
                                                }
                                                else if(symbol_table_.local_symbol_exists(func.get_function_name(), tokens[j].value_))
                                                {
                                                    cout << "调用时参数为局部变量" << endl;
                                                }
                                                else if(symbol_table_.function_parameter_exists(func.get_function_name(), tokens[j].value_))
                                                {
                                                    cout << "调用时参数为定义时的参数" << endl;
                                                }
                                                else
                                                {
                                                    cout << "undefined identifier" << endl;
                                                    throw std::exception();
                                                }
                                            }
                                            else if(tokens[j].value_ == ",")
                                            {
                                                count++;
                                            }
                                        }
                                        if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                        {
                                            cout << "参数正确" << endl;
                                        }
                                        else
                                        {
                                            cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                            throw std::exception();
                                        }
                                    }
                                }
                            }
                            else if(tokens[k].value_ == "{")
                            {
                                inner_brace++;
                            }
                            else if(tokens[k].value_ == "}")
                            {
                                inner_brace--;
                                if(inner_brace == 0)
                                {
                                    i = k + 1;
                                    break;
                                }
                            }
                            else
                            {
                                k++;
                            }
                        }
                    }
                }

            }
        }
        else if(tokens[i].type_ == "identifier")
        {
            //之前没有出现左括号
            if(left_brace == 0)
            {
                //说明为全局变量
                //类型为常变量
                if(i - 2 >= 0 && tokens[i - 2].value_ == "const")
                {
                    string type = tokens[i - 2].value_.append(" ").append(tokens[i - 1].value_);
                    string name = tokens[i].value_;
                    if(tokens[i + 1].value_ != "[")
                    {
                        Symbol sym(Category::CONST_VAR, name, type, left_brace, false);
                        sym.set_address(&sym);
                        sym.set_property("global symbol");
                        symbol_table_.emplace_global_symbol(sym);
                    }
                    else
                    {
                        Symbol sym(Category::CONST_ARRAY, name, type, left_brace, false);
                        sym.set_address(&sym);
                        sym.set_property("global array symbol");
                        symbol_table_.emplace_global_symbol(sym);
                    }
                }
                else if(i - 2 >= 0)
                {
                    string type = tokens[i - 1].value_;
                    string name = tokens[i].value_;
                    if(tokens[i + 1].value_ != "[")
                    {
                        Symbol sym(Category::CONST_VAR, name, type, left_brace, false);
                        sym.set_address(&sym);
                        sym.set_property("global symbol");
                        symbol_table_.emplace_global_symbol(sym);
                    }
                    else
                    {
                        Symbol sym(Category::CONST_ARRAY, name, type, left_brace, false);
                        sym.set_address(&sym);
                        sym.set_property("global array symbol");
                        symbol_table_.emplace_global_symbol(sym);
                    }
                }
            }
            i++;
        }
        else if(tokens[i].value_ == "struct")
        {
            if(tokens[i + 1].type_ == "identifier-struct")
            {
                StructSymbol st(Category::STRUCT, tokens[i + 1].value_, "struct", 0, false);
                st.set_address(&st);
                st.set_property("global struct");
                auto struct_name = tokens[i + 1].value_;
                symbol_table_.emplace_struct(st);
                int inner_brace = 1;
                i += 2;

                //处理 struct 体
                if(tokens[i].value_ == "{")
                {
                    for(size_t k = i + 1;k < tokens.size();)
                    {
                        //即为struct内变量声明
                        if((tokens[k - 1].value_ == "int"   ||
                            tokens[k - 1].value_ == "void"  ||
                            tokens[k - 1].value_ == "float" ||
                            tokens[k - 1].value_ == "double"||
                            tokens[k - 1].value_ == "bool") &&
                           tokens[k].type_ == "identifier")
                        {
                            if(tokens[k - 2].value_ == "const")
                            {
                                string type1 = tokens[k - 2].value_.append(" ").append(tokens[k - 1].value_);
                                string name1 = tokens[k].value_;
                                if(tokens[k + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, inner_brace, false);
                                    sym.set_property("struct member symbol");
                                    sym.set_address(&sym);
                                    symbol_table_.get_struct(st.get_struct_name()).emplace_struct_symbols(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, inner_brace, false);
                                    sym.set_property("struct member array symbol");
                                    sym.set_address(&sym);
                                    symbol_table_.get_struct(st.get_struct_name()).emplace_struct_symbols(sym);
                                }
                            }
                            else
                            {
                                string type1 = tokens[k - 1].value_;
                                string name1 = tokens[k].value_;
                                if(tokens[k + 1].value_ != "[")
                                {
                                    Symbol sym(Category::CONST_VAR, name1, type1, inner_brace, false);
                                    sym.set_property("struct member symbol");
                                    sym.set_address(&sym);
                                    symbol_table_.get_struct(st.get_struct_name()).emplace_struct_symbols(sym);
                                }
                                else
                                {
                                    Symbol sym(Category::CONST_ARRAY, name1, type1, inner_brace, false);
                                    sym.set_property("struct member array symbol");
                                    sym.set_address(&sym);
                                    symbol_table_.get_struct(st.get_struct_name()).emplace_struct_symbols(sym);
                                }
                            }
                            k++;
                        }
                        else if(tokens[k].type_ == "identifier" || tokens[k].type_ == "identifier1")    //变量使用
                        {
                            if(symbol_table_.global_symbol_exists(tokens[k].value_))
                            {
                                cout << "使用的变量为全局变量" << endl;
                            }
                            else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[k].value_))
                            {
                                cout << "函数内局部变量" << endl;
                            }
                            else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[k].value_))
                            {
                                cout << "函数参数" << endl;
                            }
                            k++;
                        }
                        else if(tokens[k].type_ == "identifier-function")
                        {
                            //需要观察 符号表中 struct 内是否存在该函数的符号项
                            if(symbol_table_.struct_function_exists(struct_name, tokens[k].value_)) //如果存在
                            {

                                if(tokens[k + 1].value_ != "(")
                                {
                                    //error handler
                                    cout << "焯" << endl;
                                    exit(1);
                                }
                                else if(tokens[k].value_ == "output")
                                {
                                    for(size_t j = k + 2;j < tokens.size();)
                                    {
                                        if(tokens[j].type_ == "identifier-function")
                                        {
                                            if(symbol_table_.struct_function_exists(struct_name, tokens[k].value_))
                                            {
                                                FunctionSymbol& cur_func = symbol_table_.get_function(tokens[j].value_);

                                                size_t count = 0;
                                                j += 2;
                                                for(size_t m = j;m < tokens.size();m++)
                                                {
                                                    if(tokens[m].value_ == ")")
                                                    {
                                                        j = m + 1;
                                                        break;
                                                    }

                                                    //如果是参数
                                                    if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                    {
                                                        if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                        {
                                                            cout << "调用时参数为全局变量" << endl;
                                                        }
                                                        else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[j].value_))
                                                        {
                                                            cout << "调用时参数为局部变量" << endl;
                                                        }
                                                        else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[j].value_))
                                                        {
                                                            cout << "调用时参数为定义时的参数" << endl;
                                                        }
                                                        else
                                                        {
                                                            cout << "undefined identifier" << endl;
                                                            throw std::exception();
                                                        }
                                                    }
                                                    else if(tokens[m].value_ == ",")
                                                    {
                                                        count++;
                                                    }
                                                }
                                                if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                {
                                                    cout << "参数正确" << endl;
                                                }
                                                else
                                                {
                                                    cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                    throw std::exception();
                                                }
                                                k = j;
                                            }
                                            break;
                                        }
                                        else if(tokens[j].type_ == "identifier")
                                        {
                                            for(size_t m = j;m < tokens.size();m++)
                                            {
                                                if(tokens[j].value_ == ",")
                                                {
                                                    cout << "参数过多" << endl;
                                                    exit(1);
                                                }
                                            }

                                            break;
                                        }
                                        else
                                        {
                                            j++;
                                        }
                                    }
                                }
                                else
                                {
                                    FunctionSymbol& cur_func = symbol_table_.get_function(tokens[k].value_);
                                    k+=2;
                                    size_t count = 0;
                                    //查看参数列表是否正确
                                    for(size_t j = k; j < tokens.size();j++)
                                    {
                                        if(tokens[j].value_ == ")")
                                        {
                                            k = j + 1;
                                            break;
                                        }
                                        //如果是参数
                                        if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                        {
                                            if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                            {
                                                cout << "调用时参数为全局变量" << endl;
                                            }
                                            else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[j].value_))
                                            {
                                                cout << "调用时参数为局部变量" << endl;
                                            }
                                            else if(symbol_table_.struct_symbol_exists(st.get_struct_name(), tokens[j].value_))
                                            {
                                                cout << "调用时参数为定义时的参数" << endl;
                                            }
                                            else
                                            {
                                                cout << "undefined identifier" << endl;
                                                throw std::exception();
                                            }
                                        }
                                        else if(tokens[j].value_ == ",")
                                        {
                                            count++;
                                        }
                                    }
                                    if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                    {
                                        cout << "参数正确" << endl;
                                    }
                                    else
                                    {
                                        cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                        throw std::exception();
                                    }
                                }
                            }
                            else //如果不存在
                            {
                                //即为函数签名
                                //获取该函数的type-specifier以及形参
                                //函数返回类型为常值
                                cout << 1 << endl;
                                if(k - 2 >= 0 && tokens[k - 2].value_ == "const")
                                {
                                    string type = tokens[k - 2].value_.append(" ").append(tokens[k - 1].value_);
                                    string name = tokens[k].value_;
                                    FunctionSymbol func(Category::FUNC, name, type);
                                    func.set_address(&func);
                                    func.set_property("struct member function");

                                    symbol_table_.get_struct(struct_name).emplace_struct_function(func);


                                    //将参数加入 parameters_中
                                    k += 2;
                                    for(size_t j = k;j < tokens.size();j++)
                                    {
                                        if(tokens[j].value_ == ")")
                                        {
                                            k = j + 1;
                                            break;
                                        }

                                        if(tokens[j].type_ == "identifier")
                                        {
                                            if(tokens[j - 2].value_ == "const")
                                            {
                                                string type1 = tokens[j - 2].value_.append(" ").append(tokens[j - 1].value_);
                                                string name1 = tokens[j].value_;
                                                if(tokens[j + 1].value_ != "[")
                                                {
                                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                                else
                                                {
                                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function array parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                            }
                                            else
                                            {
                                                string type1 = tokens[j  - 1].value_;
                                                string name1 = tokens[j].value_;
                                                if(tokens[j + 1].value_ != "[")
                                                {
                                                    Symbol sym(Category::VAR, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                                else
                                                {
                                                    Symbol sym(Category::ARRAY, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function array parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                            }
                                        }
                                    }

                                    //接下来处理该函数体
                                    int inner_inner_brace = 1;
                                    if(tokens[k].value_ == "{")
                                    {
                                        for(size_t n = k + 1;n < tokens.size();)
                                        {
                                            //即为函数局部变量声明
                                            if((tokens[n - 1].value_ == "int"   ||
                                                tokens[n - 1].value_ == "void"  ||
                                                tokens[n - 1].value_ == "float" ||
                                                tokens[n - 1].value_ == "double"||
                                                tokens[n - 1].value_ == "bool") &&
                                               tokens[n].type_ == "identifier")
                                            {
                                                if(tokens[n - 2].value_ == "const")
                                                {
                                                    string type1 = tokens[n - 2].value_.append(" ").append(tokens[n - 1].value_);
                                                    string name1 = tokens[n].value_;
                                                    if(tokens[n + 1].value_ == "[")
                                                    {
                                                        Symbol sym(Category::CONST_ARRAY, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local array symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                    else
                                                    {
                                                        Symbol sym(Category::CONST_VAR, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                }
                                                else
                                                {
                                                    string type1 = tokens[n - 1].value_;
                                                    string name1 = tokens[n].value_;
                                                    if(tokens[n + 1].value_ == "[")
                                                    {
                                                        Symbol sym(Category::ARRAY, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local array symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                    else
                                                    {
                                                        Symbol sym(Category::VAR, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                }
                                                n++;
                                            }
                                            else if(tokens[n].type_ == "identifier" || tokens[n].type_ == "identifier1")    //变量使用
                                            {
                                                if(symbol_table_.global_symbol_exists(tokens[n].value_))
                                                {
                                                    cout << "使用的变量为全局变量" << endl;
                                                }
                                                else if(symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).local_symbol_exists(tokens[n].value_))
                                                {
                                                    cout << "函数内局部变量" << endl;
                                                }
                                                else if(symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).parameter_exists(tokens[n].value_))
                                                {
                                                    cout << "函数参数" << endl;
                                                }
                                                n++;
                                            }
                                            else if(tokens[n].type_ == "identifier-function")   //struct 内函数出现函数，此时的函数即为函数调用
                                            {
                                                cout << name << endl;
                                                symbol_table_.get_struct(struct_name).get_struct_function(name).emplace_function_call(tokens[n].value_);
                                                //需要观察 符号表中是否存在该函数的符号项
                                                if(symbol_table_.function_exists(tokens[n].value_) || symbol_table_.get_struct(struct_name).function_exists(tokens[n].value_)) //如果存在
                                                {
                                                    if(tokens[n + 1].value_ != "(")
                                                    {
                                                        //error handler
                                                        cout << "焯" << endl;
                                                        exit(1);
                                                    }
                                                    else if(tokens[n].value_ == "output")
                                                    {
                                                        for(size_t j = n + 2;j < tokens.size();)
                                                        {
                                                            if(tokens[j].type_ == "identifier-function")
                                                            {
                                                                if(symbol_table_.function_exists(tokens[j].value_))
                                                                {
                                                                    auto cur_func = symbol_table_.get_function(tokens[j].value_);
                                                                    size_t count = 0;
                                                                    j += 2;
                                                                    for(size_t m = j;m < tokens.size();m++)
                                                                    {
                                                                        if(tokens[m].value_ == ")")
                                                                        {
                                                                            j = m + 1;
                                                                            break;
                                                                        }

                                                                        //如果是参数
                                                                        if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                                        {
                                                                            if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                                            {
                                                                                cout << "调用时参数为全局变量" << endl;
                                                                            }
                                                                            else if(st.get_struct_function(func.get_function_name()).local_symbol_exists(tokens[k].value_))
                                                                            {
                                                                                cout << "函数内局部变量" << endl;
                                                                            }
                                                                            else if(st.get_struct_function(func.get_function_name()).parameter_exists(tokens[k].value_))
                                                                            {
                                                                                cout << "函数参数" << endl;
                                                                            }
                                                                            else
                                                                            {
                                                                                cout << "undefined identifier" << endl;
                                                                                throw std::exception();
                                                                            }
                                                                        }
                                                                        else if(tokens[m].value_ == ",")
                                                                        {
                                                                            count++;
                                                                        }
                                                                    }
                                                                    if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                                    {
                                                                        cout << "参数正确" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                    n = j;
                                                                }
                                                                break;
                                                            }
                                                            else if(tokens[j].type_ == "identifier")
                                                            {
                                                                for(size_t m = j;m < tokens.size();m++)
                                                                {
                                                                    if(tokens[j].value_ == ",")
                                                                    {
                                                                        cout << "参数过多" << endl;
                                                                        exit(1);
                                                                    }
                                                                }

                                                                break;
                                                            }
                                                            else
                                                            {
                                                                j++;
                                                            }
                                                        }

                                                    }
                                                    else
                                                    {
                                                        if(symbol_table_.function_exists(tokens[n].value_))
                                                        {
                                                            FunctionSymbol& cur_func = symbol_table_.get_function(tokens[n].value_);
                                                            n+=2;
                                                            size_t count = 0;
                                                            //查看参数列表是否正确
                                                            for(size_t j = n; j < tokens.size();j++)
                                                            {
                                                                if(tokens[j].value_ == ")")
                                                                {
                                                                    n = j + 1;
                                                                    break;
                                                                }
                                                                //如果是参数
                                                                if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                                                {
                                                                    if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为全局变量" << endl;
                                                                    }
                                                                    else if(cur_func.local_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为局部变量" << endl;
                                                                    }
                                                                    else if(cur_func.parameter_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为定义时的参数" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "undefined identifier" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                }
                                                                else if(tokens[j].value_ == ",")
                                                                {
                                                                    count++;
                                                                }
                                                            }
                                                            if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                            {
                                                                cout << "参数正确" << endl;
                                                            }
                                                            else
                                                            {
                                                                cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                throw std::exception();
                                                            }
                                                        }
                                                        else if(symbol_table_.get_struct(struct_name).function_exists(tokens[n].value_))
                                                        {
                                                            FunctionSymbol& cur_func = symbol_table_.get_struct(struct_name).get_struct_function(tokens[n].value_);
                                                            n+=2;
                                                            size_t count = 0;
                                                            //查看参数列表是否正确
                                                            for(size_t j = n; j < tokens.size();j++)
                                                            {
                                                                if(tokens[j].value_ == ")")
                                                                {
                                                                    n = j + 1;
                                                                    break;
                                                                }
                                                                //如果是参数
                                                                if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                                                {
                                                                    if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为全局变量" << endl;
                                                                    }
                                                                    else if(cur_func.local_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为局部变量" << endl;
                                                                    }
                                                                    else if(cur_func.parameter_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为定义时的参数" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "undefined identifier" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                }
                                                                else if(tokens[j].value_ == ",")
                                                                {
                                                                    count++;
                                                                }
                                                            }
                                                            if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                            {
                                                                cout << "参数正确" << endl;
                                                            }
                                                            else
                                                            {
                                                                cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                throw std::exception();
                                                            }
                                                        }
                                                    }
                                                    n++;
                                                }
                                            }
                                            else if(tokens[n].value_ == "{")
                                            {
                                                inner_inner_brace++;
                                            }
                                            else if(tokens[n].value_ == "}")
                                            {
                                                inner_inner_brace--;
                                                if(inner_inner_brace == 0)
                                                {
                                                    k = n + 1;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                n++;
                                            }
                                        }
                                    }
                                }
                                else if(k - 2 >= 0)
                                {
                                    string type = tokens[k - 1].value_;
                                    string name = tokens[k].value_;
                                    FunctionSymbol func(Category::FUNC, name, type);
                                    func.set_address(&func);
                                    func.set_property("struct member function");

                                    symbol_table_.get_struct(struct_name).emplace_struct_function(func);

                                    //将参数加入 parameters_中
                                    k += 2;
                                    for(size_t j = k;j < tokens.size();j++)
                                    {
                                        if(tokens[j].value_ == ")")
                                        {
                                            k = j + 1;
                                            break;
                                        }

                                        if(tokens[j].type_ == "identifier")
                                        {
                                            if(tokens[j - 2].value_ == "const")
                                            {
                                                string type1 = tokens[j - 2].value_.append(" ").append(tokens[j - 1].value_);
                                                string name1 = tokens[j].value_;
                                                if(tokens[j + 1].value_ != "[")
                                                {
                                                    Symbol sym(Category::CONST_VAR, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                                else
                                                {
                                                    Symbol sym(Category::CONST_ARRAY, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function array parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                            }
                                            else
                                            {
                                                string type1 = tokens[j  - 1].value_;
                                                string name1 = tokens[j].value_;
                                                if(tokens[j + 1].value_ != "[")
                                                {
                                                    Symbol sym(Category::VAR, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                                else
                                                {
                                                    Symbol sym(Category::ARRAY, name1, type1, true);
                                                    sym.set_address(&sym);
                                                    sym.set_property("member function array parameter");
                                                    symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_parameter(sym);
                                                }
                                            }
                                        }
                                    }

                                    //接下来处理该函数体
                                    int inner_inner_brace = 1;
                                    if(tokens[k].value_ == "{")
                                    {
                                        for(size_t n = k + 1;n < tokens.size();)
                                        {
                                            //即为函数局部变量声明
                                            if((tokens[n - 1].value_ == "int"   ||
                                                tokens[n - 1].value_ == "void"  ||
                                                tokens[n - 1].value_ == "float" ||
                                                tokens[n - 1].value_ == "double"||
                                                tokens[n - 1].value_ == "bool") &&
                                               tokens[n].type_ == "identifier")
                                            {
                                                if(tokens[n - 2].value_ == "const")
                                                {
                                                    string type1 = tokens[n - 2].value_.append(" ").append(tokens[n - 1].value_);
                                                    string name1 = tokens[n].value_;
                                                    if(tokens[n + 1].value_ == "[")
                                                    {
                                                        Symbol sym(Category::CONST_ARRAY, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local array symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                    else
                                                    {
                                                        Symbol sym(Category::CONST_VAR, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                }
                                                else
                                                {
                                                    string type1 = tokens[n - 1].value_;
                                                    string name1 = tokens[n].value_;
                                                    if(tokens[n + 1].value_ == "[")
                                                    {
                                                        Symbol sym(Category::ARRAY, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local array symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                    else
                                                    {
                                                        Symbol sym(Category::VAR, name1, type1, inner_inner_brace, false);
                                                        sym.set_address(&sym);
                                                        sym.set_property("member function local symbol");
                                                        symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).emplace_local_symbol(sym);
                                                    }
                                                }
                                                n++;
                                            }
                                            else if(tokens[n].type_ == "identifier" || tokens[n].type_ == "identifier1")    //变量使用
                                            {
                                                if(symbol_table_.global_symbol_exists(tokens[n].value_))
                                                {
                                                    cout << "使用的变量为全局变量" << endl;
                                                }
                                                else if(symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).local_symbol_exists(tokens[n].value_))
                                                {
                                                    cout << "函数内局部变量" << endl;
                                                }
                                                else if(symbol_table_.get_struct(struct_name).get_struct_function(func.get_function_name()).parameter_exists(tokens[n].value_))
                                                {
                                                    cout << "函数参数" << endl;
                                                }
                                                n++;
                                            }
                                            else if(tokens[n].type_ == "identifier-function")   //struct 内函数出现函数，此时的函数即为函数调用
                                            {
                                                cout << name << endl;
                                                symbol_table_.get_struct(struct_name).get_struct_function(name).emplace_function_call(tokens[n].value_);
                                                //需要观察 符号表中是否存在该函数的符号项
                                                if(symbol_table_.function_exists(tokens[n].value_) || symbol_table_.get_struct(struct_name).function_exists(tokens[n].value_)) //如果存在
                                                {
                                                    if(tokens[n + 1].value_ != "(")
                                                    {
                                                        //error handler
                                                        cout << "焯" << endl;
                                                        exit(1);
                                                    }
                                                    else if(tokens[n].value_ == "output")
                                                    {
                                                        for(size_t j = n + 2;j < tokens.size();)
                                                        {
                                                            if(tokens[j].type_ == "identifier-function")
                                                            {
                                                                if(symbol_table_.function_exists(tokens[j].value_))
                                                                {
                                                                    auto cur_func = symbol_table_.get_function(tokens[j].value_);
                                                                    size_t count = 0;
                                                                    j += 2;
                                                                    for(size_t m = j;m < tokens.size();m++)
                                                                    {
                                                                        if(tokens[m].value_ == ")")
                                                                        {
                                                                            j = m + 1;
                                                                            break;
                                                                        }

                                                                        //如果是参数
                                                                        if(tokens[m].type_ == "identifier" || tokens[m].type_ == "identifier1")
                                                                        {
                                                                            if(symbol_table_.global_symbol_exists(tokens[m].value_))
                                                                            {
                                                                                cout << "调用时参数为全局变量" << endl;
                                                                            }
                                                                            else if(st.get_struct_function(func.get_function_name()).local_symbol_exists(tokens[k].value_))
                                                                            {
                                                                                cout << "函数内局部变量" << endl;
                                                                            }
                                                                            else if(st.get_struct_function(func.get_function_name()).parameter_exists(tokens[k].value_))
                                                                            {
                                                                                cout << "函数参数" << endl;
                                                                            }
                                                                            else
                                                                            {
                                                                                cout << "undefined identifier" << endl;
                                                                                throw std::exception();
                                                                            }
                                                                        }
                                                                        else if(tokens[m].value_ == ",")
                                                                        {
                                                                            count++;
                                                                        }
                                                                    }
                                                                    if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                                    {
                                                                        cout << "参数正确" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                    n = j;
                                                                }
                                                                break;
                                                            }
                                                            else if(tokens[j].type_ == "identifier")
                                                            {
                                                                for(size_t m = j;m < tokens.size();m++)
                                                                {
                                                                    if(tokens[j].value_ == ",")
                                                                    {
                                                                        cout << "参数过多" << endl;
                                                                        exit(1);
                                                                    }
                                                                }

                                                                break;
                                                            }
                                                            else
                                                            {
                                                                j++;
                                                            }
                                                        }

                                                    }
                                                    else
                                                    {
                                                        if(symbol_table_.function_exists(tokens[n].value_))
                                                        {
                                                            FunctionSymbol& cur_func = symbol_table_.get_function(tokens[n].value_);
                                                            n+=2;
                                                            size_t count = 0;
                                                            //查看参数列表是否正确
                                                            for(size_t j = n; j < tokens.size();j++)
                                                            {
                                                                if(tokens[j].value_ == ")")
                                                                {
                                                                    n = j + 1;
                                                                    break;
                                                                }
                                                                //如果是参数
                                                                if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                                                {
                                                                    if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为全局变量" << endl;
                                                                    }
                                                                    else if(cur_func.local_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为局部变量" << endl;
                                                                    }
                                                                    else if(cur_func.parameter_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为定义时的参数" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "undefined identifier" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                }
                                                                else if(tokens[j].value_ == ",")
                                                                {
                                                                    count++;
                                                                }
                                                            }
                                                            if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                            {
                                                                cout << "参数正确" << endl;
                                                            }
                                                            else
                                                            {
                                                                cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                throw std::exception();
                                                            }
                                                        }
                                                        else if(symbol_table_.get_struct(struct_name).function_exists(tokens[n].value_))
                                                        {
                                                            FunctionSymbol& cur_func = symbol_table_.get_struct(struct_name).get_struct_function(tokens[n].value_);
                                                            n+=2;
                                                            size_t count = 0;
                                                            //查看参数列表是否正确
                                                            for(size_t j = n; j < tokens.size();j++)
                                                            {
                                                                if(tokens[j].value_ == ")")
                                                                {
                                                                    n = j + 1;
                                                                    break;
                                                                }
                                                                //如果是参数
                                                                if(tokens[j].type_ == "identifier" || tokens[j].type_ == "identifier1")
                                                                {
                                                                    if(symbol_table_.global_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为全局变量" << endl;
                                                                    }
                                                                    else if(cur_func.local_symbol_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为局部变量" << endl;
                                                                    }
                                                                    else if(cur_func.parameter_exists(tokens[j].value_))
                                                                    {
                                                                        cout << "调用时参数为定义时的参数" << endl;
                                                                    }
                                                                    else
                                                                    {
                                                                        cout << "undefined identifier" << endl;
                                                                        throw std::exception();
                                                                    }
                                                                }
                                                                else if(tokens[j].value_ == ",")
                                                                {
                                                                    count++;
                                                                }
                                                            }
                                                            if(count == (cur_func.parameters_size() - 1) || (count == 0 && cur_func.parameters_size() == 0))
                                                            {
                                                                cout << "参数正确" << endl;
                                                            }
                                                            else
                                                            {
                                                                cout << "function expected " << cur_func.parameters_size() << ", but only has " << count + 1 << " arguments" << endl;
                                                                throw std::exception();
                                                            }
                                                        }
                                                    }
                                                    n++;
                                                }
                                            }
                                            else if(tokens[n].value_ == "{")
                                            {
                                                inner_inner_brace++;
                                            }
                                            else if(tokens[n].value_ == "}")
                                            {
                                                inner_inner_brace--;
                                                if(inner_inner_brace == 0)
                                                {
                                                    k = n + 1;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                n++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if(tokens[k].value_ == "{")
                        {
                            inner_brace++;
                            k++;
                        }
                        else if(tokens[k].value_ == "}")
                        {
                            inner_brace--;
                            k++;
                            if(inner_brace == 0)
                            {
                                i = k + 1;
                                break;
                            }
                        }
                        else
                        {
                            k++;
                        }
                    }
                }
            }
        }
        else if(tokens[i].value_ == "{")
        {
            left_brace++;
            i++;
        }
        else if(tokens[i].value_ == "}")
        {
            left_brace--;
            i++;
        }
        else
        {
            i++;
        }

    }

    // 用于测试
    //symbol_table_.print_table();

    symbol_table_.display(file_out_);

}
