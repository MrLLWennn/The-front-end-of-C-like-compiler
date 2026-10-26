//
// 创建者：86198；日期：2024/10/20。
//

#include "parser.h"
#include <algorithm>


void my_compiler::Parser::generate_grammatical_analysis_tree_aux()
{
    //产生式推导预处理
    // 如果产生式中连续两个推导的左部 相同，即推导相同的非终结符号
    // 且第二个产生式的右部 是 empty ，即产生式应该推导为 非终结符号 -> empty
    // 需要删除第一个产生式
    // 为避免 infer_访问越界，且最后一个推导为直接推导  $ -> empty, 直接跳过最后一个的判断
    if (tree_.infer_.empty())
        throw std::logic_error("cannot build a tree without productions");
    for(size_t i = 0;i + 1 < tree_.infer_.size();)
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
                                              [cur](const string& value)
                                              {
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

    // 新文法把声明、后缀和赋值分开，不再按“后面是否为=”篡改标识符类别。
    const bool declarators = is_non_terminal("<declarator-stars>");
    vector<string> input;
    for(size_t i = 0; i < tokens_.size(); ++i)
    {
        const auto value = tokens_[i].get_attribute_value();
        if(tokens_[i].is_string())
            input.emplace_back("string-literal");
        else if(tokens_[i].is_character())
            input.emplace_back("character-literal");
        else if(tokens_[i].is_number())
            input.emplace_back("number");
        else if(tokens_[i].is_identifier() && declarators)
            input.emplace_back("identifier");
        else if(tokens_[i].is_identifier())
        {
            if(i > 0 && tokens_[i - 1].get_attribute_value() == "struct")
                input.emplace_back("identifier-struct");
            else if(i > 0 && tokens_[i - 1].get_attribute_value() == ".")
                input.emplace_back("identifier");
            else if(i + 1 < tokens_.size() && tokens_[i + 1].get_attribute_value() == "(")
                input.emplace_back("identifier-function");
            else
            {
                // p.inner.x = ...归类的是根对象p，成员名字保持identifier。
                auto after = i + 1;
                while(after + 1 < tokens_.size() && tokens_[after].get_attribute_value() == "." &&
                      tokens_[after + 1].is_identifier())
                    after += 2;
                const auto next = after < tokens_.size() ? tokens_[after].get_attribute_value() : "";
                input.emplace_back(next == "=" || next == "+=" || next == "-=" || next == "/=" || next == "*="
                    ? "identifier1" : "identifier");
            }
        }
        else
            input.push_back(value);
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



    const auto peek = [&input](std::size_t index) -> string
    {
        return index < input.size() ? input[index] : "$";
    };
    const auto fail = [this](const string& code, const string& message,
                             std::size_t index) -> void
    {
        const auto line = index < tokens_.size() ? tokens_[index].get_line() : 0;
        throw CompilationError(code, pretreatment_file, line, 0, message);
    };
    // 因子化可以把终结符期待变成非终结符选择，诊断不能依赖派生名字。
    // 只有非空表项归并后指向同一种用户可读 Token，才报告明确的 Token 缺失。
    const auto expected_token = [](const auto& row) -> string
    {
        vector<string> labels;
        for(const auto& entry : row)
        {
            if(entry.second.empty())
                continue;
            const auto& token = entry.first;
            if(token == "identifier" || token == "identifier1" ||
               token == "identifier-function" || token == "identifier-struct")
                labels.emplace_back("identifier");
            else
                labels.push_back(token);
        }
        std::sort(labels.begin(), labels.end());
        labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
        return labels.size() == 1 ? labels.front() : string{};
    };
    if (input.empty())
        fail("E_PARSE_EMPTY", "expected a declaration; input is empty", 0);

    std::size_t cur = 0;
    while (stack_.top() != "$")
    {
        string top = stack_.top();
        const string symbol = peek(cur);
        if (cur == input.size())
        {
            // 仅当栈中剩余符号都可空时，才允许输入结束。
            // 沿用旧实现的直接 empty 规则，不临时编造产生式。
            const auto& alternatives = predictive_analysis_table_.productions_[top];
            if (std::find(alternatives.begin(), alternatives.end(), "empty") == alternatives.end())
                fail("E_PARSE_EOF", "unexpected end of input while expecting " + top, cur);
            stack_.pop();
            productions_.emplace_back(top + " -> empty");
            continue;
        }
        const auto next = cur + 1;
        const auto next_next = next + 1;

        if(top == symbol)
        {
            stack_.pop();
            ++cur;
        }
        else if(declarators && top == "<external>")
        {
            const string rule = symbol == "struct" && peek(cur + 2) == "{"
                ? "<record-definition>" : "<function-or-object>";
            stack_.pop();
            stack_.emplace(rule);
            productions_.emplace_back(top + " -> " + rule);
        }
        else if(declarators && top == "<parameters>" && symbol == "void" && peek(next) == ")")
        {
            stack_.pop();
            stack_.emplace("void");
            productions_.emplace_back(top + " -> void");
        }
        else if(declarators && top == "<unary>" && symbol == "(")
        {
            const auto look = peek(next);
            const bool cast = look == "int" || look == "char" || look == "bool" || look == "float" ||
                              look == "double" || look == "void" || look == "struct";
            const string rule = cast ? "( <base-type> <declarator-stars> ) <unary>" : "<postfix>";
            stack_.pop();
            const auto parts = split_string(rule);
            for(auto i = parts.rbegin(); i != parts.rend(); ++i) stack_.emplace(*i);
            productions_.emplace_back(top + " -> " + rule);
        }
        else if(declarators && top == "<else-opt>" && symbol == "else")
        {
            stack_.pop();
            stack_.emplace("<stmt>");
            stack_.emplace("else");
            productions_.emplace_back(top + " -> else <stmt>");
        }
        else if(top == "<declaration>" && symbol == "struct")
        {
            // struct Tag { ... };、struct Tag f(...)和struct Tag value分别分派。
            const string rule = peek(next_next) == "{" ? "<struct-declaration>" :
                peek(next_next) == "identifier-function" ? "<func-declaration>" : "<var-declaration>";
            stack_.pop();
            stack_.emplace(rule);
            productions_.emplace_back(top + " -> " + rule);
        }
        else if(top == "<declaration>" && peek(next_next) == "identifier-function" && input[cur] == "const")
        {
            stack_.pop();
            stack_.emplace("<func-declaration>");
            string prod = top.append(" -> <func-declaration>");
            productions_.emplace_back(prod);
        }
        else if(top == "<declaration>" && peek(next) == "identifier-function") //区分一般标识符和 函数标识符
        {
            auto vec = predictive_analysis_table_.productions_[top];
            auto iter = std::find(vec.begin(), vec.end(), "<func-declaration>");
            stack_.pop();
            if (iter == vec.end())
                fail("E_PARSE_RULE", "missing function declaration rule", cur);
            stack_.emplace(*iter);
            string prod = top.append(" -> ").append(*iter);
            productions_.emplace_back(prod);
        }
        else if(top == "<param>" && peek(next_next) == "[" && peek(next_next + 1) == "]")
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
        else if(top == "<params>" && symbol == "void" && peek(next) == ")")
        {
            //此时 应该将 <params> 解析为 25 <params> -> void
            stack_.pop();
            stack_.emplace("void");
            string prod = top.append(" -> ").append(symbol);
            productions_.emplace_back(prod);
        }
        else if(top == "<params>" && peek(next) == ")")
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
        else if(top == "<statement-list>'" && symbol != "}")
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
        else if(top == "<expression>" && (peek(next) == "++" || peek(next) == "--"))
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
                symbol != "float" &&
                symbol != "char" &&
                symbol != "void" &&
                symbol != "struct"))
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
                symbol != "float" &&
                symbol != "char" &&
                symbol != "void" &&
                symbol != "struct"))
        {
            //<assign-expression> -> <expression>
            stack_.pop();
            stack_.emplace("<expression>");
            string prod = top.append(" -> <expression>");
            productions_.emplace_back(prod);
        }
        else if(is_terminal(top))
        {
            fail("E_PARSE_TOKEN", "expected '" + top + "', got '" + symbol + "'", cur);
        }
        else if(predictive_analysis_table_.table_[top][symbol].empty())
        {
            const auto& alternatives = predictive_analysis_table_.productions_[top];
            if (std::find(alternatives.begin(), alternatives.end(), "empty") != alternatives.end())
            {
                // 保留旧实现的可空回退；本轮诊断改造不宣称
                // 已消除文法中的全部预测冲突。
                stack_.pop();
                productions_.emplace_back(top + " -> empty");
            }
            else
            {
                const auto expected = expected_token(predictive_analysis_table_.table_.at(top));
                if(!expected.empty())
                    fail("E_PARSE_TOKEN", "expected '" + expected + "', got '" + symbol + "'", cur);
                fail("E_PARSE_TABLE", "no production for " + top + " with '" + symbol + "'", cur);
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

        file_out_ << stack_;
        file_out_ << "     ---     "  << "当前输入符号为：" << symbol << endl;
    }
    file_out_ << "预测分析结束" << endl;

    if (cur != input.size())
        fail("E_PARSE_TRAILING", "unexpected trailing token '" + peek(cur) + "'", cur);

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
