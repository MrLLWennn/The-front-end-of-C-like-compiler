//
// Created by 86198 on 2024/10/20.
//

#include "parser.h"


void my_compiler::Parser::generate_grammatical_analysis_tree_aux()
{
    //产生式推导预处理
    // 如果产生式中连续两个推导的左部 相同，即推导相同的非终结符号
    // 且第二个产生式的右部 是 empty ，即产生式应该推导为 非终结符号 -> empty
    // 需要删除第一个产生式
    // 为避免 infer_访问越界，且最后一个推导为直接推导  $ -> empty, 直接跳过最后一个的判断
    for(size_t i = 0;i < tree_.infer_.size() - 1;)
    {
        string cur = tree_.infer_[i];
        size_t index1 = cur.find("->");
        string cur_left = cur.substr(0, index1);
        remove_front_back_blank(cur_left);
        size_t index2 = tree_.infer_[i + 1].find("->");
        string next_left = tree_.infer_[i + 1].substr(0, index2);
        string next_right = tree_.infer_[i + 1].substr(index2 + 2);
        remove_front_back_blank(next_left);
        remove_front_back_blank(next_right);
        if(cur_left == next_left && next_right == "empty")  //如果满足
        {
            //将当前产生式删除，保留 推导出为 empty 的
            tree_.infer_.erase(std::remove_if(tree_.infer_.begin(), tree_.infer_.end(),
                                              [cur](const string& value) {
                                                  return value == cur; // 删除当前产生式
                                              }), tree_.infer_.end());
            i += 2;
            if(i >= tree_.infer_.size())
            {
                break;
            }
        }
        else
        {
            i++;
        }
    }


    size_t index = tree_.infer_[0].find("->");
    string left = tree_.infer_[0].substr(0, index);
    string right = tree_.infer_[0].substr(index + 2);
    remove_front_back_blank(left);
    remove_front_back_blank(right);
    int op = 1;
    //对于第一个产生式 <S> -> <program> 特殊处理
    tree_.root_ = std::make_shared<Node>(op++, left, false);
    auto right_node = std::make_shared<Node>(op++, right, false);
    tree_.root_->child_.emplace_back(right_node);
    right_node->parent_ = tree_.root_;
    auto cur = tree_.root_->get_node(right);
    file_out_ << cur << "，其父节点为：" << cur->parent_ << endl;
    vector<shared_ptr<Node>> nodes;

    for(size_t i = 1;i < tree_.infer_.size();i++)
    {
        size_t index1 = tree_.infer_[i].find("->");
        string right1 = tree_.infer_[i].substr(index1 + 2);
        remove_front_back_blank(right1);
        vector<string> split = split_string(right1);
        if(split.size() == 1)
        {
            if(is_terminal(split[0]))
            {
                auto node = std::make_shared<Node>(op++, split[0], true);
                file_out_ << node << "，其父节点为：" ;
                cur->child_.push_back(node);
                node->parent_ = cur;
                file_out_ << node->parent_ << endl;
            }
            else if(is_non_terminal(split[0]))
            {
                auto node = std::make_shared<Node>(op++, split[0], false);
                file_out_ << node << "，其父节点为：";
                cur->child_.push_back(node);
                node->parent_ = cur;
                file_out_ << node->parent_ <<endl;
                nodes.emplace_back(node);
            }
        }
        else
        {
            for(const auto& s : split)
            {
                if(is_terminal(s))
                {
                    auto node = std::make_shared<Node>(op++, s, true);
                    file_out_ << node << "，其父节点为：";
                    cur->child_.push_back(node);
                    node->parent_ = cur;
                    file_out_ << node->parent_ << endl;
                }
                else if(is_non_terminal(s))
                {
                    auto node = std::make_shared<Node>(op++, s, false);
                    file_out_ << node << "，其父节点为：";
                    cur->child_.push_back(node);
                    node->parent_ = cur;
                    file_out_ << node->parent_ << endl;
                    nodes.emplace_back(node);
                }
            }
        }
        if(i < (tree_.infer_.size() - 1))
        {

            size_t index2 = tree_.infer_[i + 1].find("->");
            string next_left = tree_.infer_[i + 1].substr(0, index2);
            remove_front_back_blank(next_left);

            //如果是 连续推导出 empty
            //如 ： <var>' -> empty
            //<term>' -> empty
            //<additive-expression>' -> empty
            //<simple-expression>' -> empty
            //<arg-list>' -> empty
            //<term>' -> empty
            //<additive-expression>' -> empty
            //<simple-expression>' -> empty
            //<arg-list>' -> empty
            //<term>' -> empty
            //则 需要从后先前遍历，删除 nodes 中最后加入的且 value_ == left 的 node;
            for(int j = nodes.size() - 1;j >= 0;j--)
            {
                if(nodes[j]->value_ == next_left)
                {
                    cur = nodes[j];
                    nodes.erase(std::remove(nodes.begin(), nodes.end(), nodes[j]), nodes.end());
                    break;
                }
            }
        }
    }
}


void my_compiler::Parser::error_aux()
{

}


void my_compiler::Parser::predictive_analysis_aux()
{

    //保存语法预测分析推导的各个产生式, 用于构建语法分析树
    vector<string> productions_;

    vector<string> input;
    for(size_t i = 0;i < tokens_.size();i++)
    {
        if(tokens_[i].is_number())
        {
            input.emplace_back("number");
        }
        else if(tokens_[i].is_identifier() && tokens_[i + 1].get_attribute_value() == "(")  //函数标识符
        {
            input.emplace_back("identifier-function");
        }
        else if(tokens_[i].is_identifier() && tokens_[i - 1].get_attribute_value() == "struct")
        {
            input.emplace_back("identifier-struct");
        }
        else if(tokens_[i].is_identifier() && (tokens_[i + 1].get_attribute_value() == "="
                                            || tokens_[i + 1].get_attribute_value() == "+="
                                            || tokens_[i + 1].get_attribute_value() == "-="
                                            || tokens_[i + 1].get_attribute_value() == "/="
                                            || tokens_[i + 1].get_attribute_value() == "*=")) //变量1 标识符
        {
            input.emplace_back("identifier1");
        }
        else if(tokens_[i].is_identifier())
        {
            input.emplace_back("identifier");
        }
        else    //对于关键字和定界符
        {
            input.emplace_back(tokens_[i].get_attribute_value());
        }
    }

    for (const auto& non : predictive_analysis_table_.Vn_)
    {
        for (const auto& ter : predictive_analysis_table_.Vt_)
        {
            auto productions = predictive_analysis_table_.table_[non][ter];
            if(productions.size() <= 1)
            {
                continue;
            }
            for(size_t i = 0;i < productions.size();i++)
            {
                if(productions[i] == "empty" && i < (productions.size() - 1))
                {
                    std::swap(productions[i], productions[productions.size() - 1]);
                }
            }
        }
    }



    struct temp_struct
    {
        vector<Stack<string>> stacks_;
        vector<int> curs_;
    };


    temp_struct history;

    string top = stack_.top();
    auto cur = 0;
    file_out_ << stack_;
    string symbol = input[cur];
    file_out_ << "     ---     "  << "当前输入符号为：" << symbol << endl;
    history.stacks_.emplace_back(stack_);
    history.curs_.emplace_back(cur);
    while(top != "$" && cur <= input.size())
    {

        if(cur == input.size())
        {
            if(stack_.size() >= 2)
            {
                bool empty = false;
                while(stack_.top() != "$")
                {
                    for(const auto& p : predictive_analysis_table_.productions_[stack_.top()])
                    {
                        if(p == "empty")    //能直接推出empty
                        {
                            //直接弹栈
                            empty = true;
                            stack_.pop();
                            file_out_ << stack_;
                            file_out_ << "     ---     "  << "当前输入符号为：$" << endl;
                            break;
                        }
                    }
                    if(!empty)
                    {
                        cout << "错误" << endl;
                        exit(1);
                    }
                }
            }
            break;
        }

        top = stack_.top();
        symbol = input[cur];
        auto next = cur + 1;
        auto next_next = next + 1;

        if(top == symbol)
        {
            stack_.pop();
            ++cur;
        }
        else if(top == "<declaration>" && input[next_next] == "identifier-function" && input[cur] == "const")
        {
            stack_.pop();
            stack_.emplace("<func-declaration>");
            string prod = top.append(" -> <func-declaration>");
            productions_.emplace_back(prod);
        }
        else if(top == "<declaration>" && input[next] == "identifier-function") //区分一般标识符和 函数标识符
        {
            auto vec = predictive_analysis_table_.productions_[top];
            auto iter = std::find(vec.begin(), vec.end(), "<func-declaration>");
            stack_.pop();
            stack_.emplace(*iter);
            string prod = top.append(" -> ").append(*iter);
            productions_.emplace_back(prod);
        }
        else if(top == "<param>" && input[next_next] == "[" && input[next_next + 1] == "]")
        {
            if(symbol != "const")
            {
                stack_.pop();
                stack_.emplace("]");
                stack_.emplace("[");
                stack_.emplace("identifier");
                stack_.emplace("<type-specifier>");
                string prod = top.append(" -> ").append("<type-specifier> identifier [ ]");
                productions_.emplace_back(prod);
            }
            else
            {
                stack_.pop();
                stack_.emplace("]");
                stack_.emplace("[");
                stack_.emplace("identifier");
                stack_.emplace("<type-specifier>");
                stack_.emplace("const");
                string prod = top.append(" -> ").append("const <type-specifier> identifier [ ]");
                productions_.emplace_back(prod);
            }
        }
        else if(top == "<params>" && symbol == "void" && input[next] == ")")
        {
            //此时 应该将 <params> 解析为 25 <params> -> void
            stack_.pop();
            stack_.emplace("void");
            string prod = top.append(" -> ").append(symbol);
            productions_.emplace_back(prod);
        }
        else if(top == "<params>" && input[next] == ")")
        {
            //此时 应该将 <params> 解析为 25 <params> -> empty
            stack_.pop();
            string prod = top.append(" -> empty");
            productions_.emplace_back(prod);
        }
        else if(top == "<type-specifier>" && symbol == "const")
        {
            //替换为 <type-specifier> -> <const-type>
            stack_.pop();
            stack_.emplace("<const-type>");
            string prod = top.append(" -> <const-type>");
            productions_.emplace_back(prod);
        }
        else if(top == "<case-stmt-list>'" && symbol == "default")
        {
            // <case-stmt-list>' -> empty
            stack_.pop();
            string prod = top.append(" -> empty");
            productions_.emplace_back(prod);
        }
        else if(top == "<in-de-expression>" && symbol == "identifier")
        {
            //<in-de-expression> -> <var> <in-de>
            stack_.pop();
            stack_.emplace("<in-de>");
            stack_.emplace("<var>");
            string prod = top.append(" -> <var> <in-de>");
            productions_.emplace_back(prod);
        }
        else if(top == "<declaration-list>'" && symbol == "}")
        {
            // <declaration-list>' -> empty
            stack_.pop();
            string prod = top.append(" -> empty");
            productions_.emplace_back(prod);
        }
        else if(top == "<expression>" && (symbol == "true" || symbol == "false"))
        {
            //替换为 <expression> -> <boolean>
            stack_.pop();
            stack_.emplace("<boolean>");
            string prod = top.append(" -> ").append(symbol);
            productions_.emplace_back(prod);
        }
        else if(top == "<statement-list>'" && input[next] != "}")
        {
            //51 <statement-list>' -> <statement> <statement-list>'
            stack_.pop();
            stack_.emplace("<statement-list>'");
            stack_.emplace("<statement>");
            string prod = top.append(" -> <statement> <statement-list>'");
            productions_.emplace_back(prod);
        }
        else if(top == "<first-expression>" && (symbol == "identifier" || symbol == "identifier1"))
        {
            // <assign-expression> -> <expression>
            stack_.pop();
            stack_.emplace("<expression>");
            string prod = top.append(" -> <expression>");
            productions_.emplace_back(prod);
        }
        else if(top == "<expression>" && (input[next] == "++" || input[next] == "--"))
        {
            //<expression> -> <in-de-expression>
            stack_.pop();
            stack_.emplace("<in-de-expression>");
            string prod = top.append(" -> <in-de-expression>");
            productions_.emplace_back(prod);
        }
        else if(top == "<local-declarations>'" &&
               (symbol != "int" &&
                symbol != "double" &&
                symbol != "bool" &&
                symbol != "float"))
        {
            //<local-declaration>' -> empty
            stack_.pop();
            string prod = top.append(" -> empty");
            productions_.emplace_back(prod);
        }
        else if(top == "<assign-expression>" &&
               (symbol != "int" &&
                symbol != "double" &&
                symbol != "bool" &&
                symbol != "float"))
        {
            //<assign-expression> -> <expression>
            stack_.pop();
            stack_.emplace("<expression>");
            string prod = top.append(" -> <expression>");
            productions_.emplace_back(prod);
        }
        else if(is_terminal(top))
        {
            cout << "产生式出错" << endl;
            cout << "发生错误，开始恢复..." << std::endl;

            //对于本次错误，进行回溯
            stack_ = history.stacks_[history.stacks_.size() - 2];
            cur = history.curs_[history.curs_.size() - 2];
            history.stacks_.pop_back();
            history.curs_.pop_back();


            //对于回溯后的栈顶（一定为非终结符号）
            string temp_top = stack_.top();
            //如果其对应的产生式推导出不同（>= 2）的情况
            if(predictive_analysis_table_.productions_[temp_top].size() >= 2)
            {
                auto iter = std::find(predictive_analysis_table_.productions_[temp_top].begin(),
                                      predictive_analysis_table_.productions_[temp_top].end(),
                                      "empty");
                //能推出 empty
                if(iter != predictive_analysis_table_.productions_[temp_top].end())
                {
                    //直接弹栈，相当于 推出 empty
                    stack_.pop();
                    string prod = temp_top.append(" -> empty");
                    productions_.emplace_back(prod);
                }
            }
            else
            {

                // 定义同步符号集合
                std::set<std::string> sync_tokens = {";", ","}; // 可以根据需要添加其他符号

                // 丢弃输入符号直到找到同步符
                while (cur < input.size() && sync_tokens.find(input[cur]) == sync_tokens.end())
                {
                    std::cout << "丢弃符号: " << input[cur] << std::endl;
                    ++cur; // 丢弃当前符号，移动到下一个符号
                }

                // 找到同步符后，继续分析
                if (cur < input.size())
                {
                    std::cout << "找到同步符: " << input[cur] << std::endl;
                    // 丢弃同步符，可以选择保留以继续分析
                    ++cur;
                }

                // 重新检查栈顶元素与当前输入符号
                if (!stack_.empty())
                {
                    top = stack_.top();
                    std::cout << "恢复后：当前栈顶元素：" << top << ", 当前输入符号为：" << input[cur] << std::endl;
                }
                else
                {
                    std::cout << "栈为空，终止分析" << std::endl;
                }
            }
        }
        else if(predictive_analysis_table_.table_[top][symbol].empty())
        {
            cout << "当前预测分析表为空" << endl;
            auto vec = predictive_analysis_table_.productions_[top];
            auto iter = std::find(vec.begin(), vec.end(), "empty");
            if(iter != vec.end())
            {
                stack_.pop();
                string prod = top.append(" -> empty");
                productions_.emplace_back(prod);
            }
            else
            {
                //对于本次错误，进行回溯
                stack_ = history.stacks_[history.stacks_.size() - 2];
                cur = history.curs_[history.curs_.size() - 2];
                history.stacks_.pop_back();
                history.curs_.pop_back();


                //对于回溯后的栈顶（一定为非终结符号）
                string temp_top = stack_.top();
                //如果其对应的产生式推导出不同（>= 2）的情况
                if(predictive_analysis_table_.productions_[temp_top].size() >= 2)
                {
                    auto iter1 = std::find(predictive_analysis_table_.productions_[temp_top].begin(),
                                          predictive_analysis_table_.productions_[temp_top].end(),
                                          "empty");
                    //能推出 empty
                    if(iter1 != predictive_analysis_table_.productions_[temp_top].end())
                    {
                        //直接弹栈，相当于 推出 empty
                        stack_.pop();
                        string prod = temp_top.append(" -> empty");
                        productions_.emplace_back(prod);
                    }
                }
                else
                {
                    cout << "发生错误，开始恢复..." << std::endl;

                    // 定义同步符号集合
                    std::set<std::string> sync_tokens = {";", ","}; // 可以根据需要添加其他符号

                    // 丢弃输入符号直到找到同步符
                    while (cur < input.size() && sync_tokens.find(input[cur]) == sync_tokens.end())
                    {
                        std::cout << "丢弃符号: " << input[cur] << std::endl;
                        ++cur; // 丢弃当前符号，移动到下一个符号
                    }

                    // 找到同步符后，继续分析
                    if (cur < input.size())
                    {
                        std::cout << "找到同步符: " << input[cur] << std::endl;
                        // 丢弃同步符，可以选择保留以继续分析
                        ++cur;
                    }

                    // 重新检查栈顶元素与当前输入符号
                    if (!stack_.empty())
                    {
                        top = stack_.top();
                        std::cout << "恢复后：当前栈顶元素：" << top << ", 当前输入符号为：" << input[cur] << std::endl;
                    }
                    else
                    {
                        std::cout << "栈为空，终止分析" << std::endl;
                    }
                }
            }
        }
        else
        {
            string prod = predictive_analysis_table_.table_[top][symbol][0];
            productions_.emplace_back(prod);
            stack_.pop();
            size_t index = prod.find("->");
            string right = prod.substr(index + 2);
            vector<string> split = split_string(right);
            if(split.size()  == 1)
            {
                stack_.emplace(split[0]);
            }
            else
            {
                for(auto iter = split.rbegin(); iter != split.rend();iter++)
                {
                    stack_.emplace(*iter);
                }
            }
        }

        if(cur == input.size() && stack_.size_ >= 2)    //输入符号分析结束，但是栈不为空（除了 $ 之外）
        {
            string prod = stack_.top().append(" -> empty");
            productions_.emplace_back(prod);
        }

        file_out_ << stack_;
        file_out_ << "     ---     "  << "当前输入符号为：" << symbol << endl;
        history.stacks_.emplace_back(stack_);
        history.curs_.emplace_back(cur);
    }
    file_out_ << "预测分析结束" << endl;

    for(auto& p : productions_)
    {
        remove_front_back_blank(p);
    }

    tree_.infer_ = productions_;
}



bool my_compiler::Parser::is_non_terminal(const string& value)
{
    auto iter = std::find(predictive_analysis_table_.Vn_.begin(), predictive_analysis_table_.Vn_.end(), value);
    return iter != predictive_analysis_table_.Vn_.end();
}


bool my_compiler::Parser::is_terminal(const string &value)
{
    auto iter = std::find(predictive_analysis_table_.Vt_.begin(), predictive_analysis_table_.Vt_.end(), value);
    return iter != predictive_analysis_table_.Vt_.end();
}



