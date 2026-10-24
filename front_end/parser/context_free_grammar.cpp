//
// 创建者：86198；日期：2024/11/1。
//
#include "context_free_grammar.h"

#include <stdexcept>
#include <sstream>
#include <utility>

namespace
{
    bool append_unique(std::vector<std::string>& target, const std::string& value)
    {
        if (std::find(target.begin(), target.end(), value) != target.end())
        {
            return false;
        }
        target.push_back(value);
        return true;
    }
    using Symbols = std::vector<std::string>;

    Symbols grammar_symbols(const std::string& production)
    {
        std::istringstream input(production);
        Symbols result;
        std::string symbol;
        while(input >> symbol)
            result.push_back(symbol);
        return result;
    }

    std::string symbol_sequence(const Symbols& symbols)
    {
        if(symbols.empty())
            return "empty";
        std::ostringstream out;
        for(std::size_t i = 0; i < symbols.size(); ++i)
        {
            if(i != 0)
                out << ' ';
            out << symbols[i];
        }
        return out.str();
    }

    std::size_t common_prefix_size(const std::vector<Symbols>& alternatives)
    {
        std::size_t count = alternatives.front().size();
        for(std::size_t i = 1; i < alternatives.size(); ++i)
        {
            count = std::min(count, alternatives[i].size());
            std::size_t matched = 0;
            while(matched < count && alternatives.front()[matched] == alternatives[i][matched])
                ++matched;
            count = matched;
        }
        return count;
    }

    std::string fresh_factor_name(const std::string& left,
                                  std::unordered_set<std::string>& used_names)
    {
        auto name = left + "'";
        while(!used_names.insert(name).second)
            name += "'";
        return name;
    }

    // 先按完整首符号分组，再提取组内符号前缀；对子规则递归处理。
    // identifier 与 identifier1 始终是两个符号，不能拆出字符尾部 1。
    void write_factored_rule(const std::string& left, const std::vector<Symbols>& alternatives,
                             std::unordered_set<std::string>& used_names, std::ostream& out)
    {
        std::vector<std::vector<Symbols>> groups;
        std::unordered_map<std::string, std::size_t> indexes;
        for(const auto& alternative : alternatives)
        {
            const auto key = alternative.empty() ? std::string{} : alternative.front();
            const auto inserted = indexes.emplace(key, groups.size());
            if(inserted.second)
                groups.emplace_back();
            auto& group = groups[inserted.first->second];
            if(std::find(group.begin(), group.end(), alternative) == group.end())
                group.push_back(alternative);
        }

        struct ChildRule
        {
            std::string left;
            std::vector<Symbols> alternatives;
        };
        std::vector<Symbols> right;
        std::vector<ChildRule> children;
        for(const auto& group : groups)
        {
            if(group.size() == 1)
            {
                right.push_back(group.front());
                continue;
            }
            const auto count = common_prefix_size(group);
            const auto child_name = fresh_factor_name(left, used_names);
            Symbols prefix(group.front().begin(), group.front().begin() + count);
            prefix.push_back(child_name);
            right.push_back(prefix);
            ChildRule child{child_name, {}};
            for(const auto& alternative : group)
                child.alternatives.emplace_back(alternative.begin() + count, alternative.end());
            children.push_back(std::move(child));
        }

        out << left << " -> ";
        for(std::size_t i = 0; i < right.size(); ++i)
        {
            if(i != 0)
                out << " | ";
            out << symbol_sequence(right[i]);
        }
        out << '\n';
        for(const auto& child : children)
            write_factored_rule(child.left, child.alternatives, used_names, out);
    }
} // namespace


void CFG::ContextFreeGrammar::remove_direct_left_recursion_and_factor_aux(
    const string &from_file, const string &to_file)
{

    input_file(file_in_, from_file);
    output_file(file_out_, to_file);

    // 先登记原文法的全部左部，防止新因子与已有派生非终结符重名。
    std::unordered_set<std::string> used_names;
    std::string original_line;
    while(std::getline(file_in_, original_line))
    {
        const auto arrow = original_line.find("->");
        if(arrow != std::string::npos)
        {
            const auto left_symbols = grammar_symbols(original_line.substr(0, arrow));
            if(!left_symbols.empty())
                used_names.insert(left_symbols.front());
        }
    }
    file_in_.clear();
    file_in_.seekg(0);

    // 读入的一行 temp
    string temp, part1, part2, p2_1, p2_2;
    size_t split, index;
    while (std::getline(file_in_, temp))
    {
        if (file_in_.fail())
        {
            break;
        }
        index = temp.find('|');
        if (index != string::npos)
        {
            split = temp.find("->");
            part1.assign(temp, 0, split);
            part2.assign(temp, split + 2, temp.length() - split - 1);
            remove_front_back_blank(part1);
            remove_front_back_blank(
                part2); // 将每一行语法分为箭头前后的两个部分
            index = part2.find('|');
            p2_1.assign(part2, 0, index);
            p2_2.assign(part2, index + 1, part2.length() - index);
            remove_front_back_blank(p2_1);
            remove_front_back_blank(
                p2_2); // 将part2部分按照|前后分为两部分
            if (p2_1.find(part1) == 0 && p2_1[part1.length()] == ' ')
            {
                p2_1.assign(p2_1, part1.length() + 1,
                            p2_1.length() - part1.length());
                const auto recursive_name = fresh_factor_name(part1, used_names);
                file_out_ << part1 << " -> ";
                if (p2_2 != "empty")
                    file_out_ << p2_2 << " ";
                else
                    file_out_ << p2_2 << " | ";
                file_out_ << recursive_name << endl;
                file_out_ << recursive_name << " -> " << p2_1 << " "
                          << recursive_name << " | empty" << endl;
            } // p2_1出现part1说明出现了左递归
            else
            {
                std::vector<Symbols> alternatives;
                std::istringstream input(part2);
                std::string alternative;
                while(std::getline(input, alternative, '|'))
                    alternatives.push_back(grammar_symbols(alternative));
                write_factored_rule(part1, alternatives, used_names, file_out_);
            }
        }
        else
        {
            file_out_ << temp << endl;
        }
    }

    file_in_.close();
    file_out_.close();
}

void CFG::ContextFreeGrammar::split_grammar_aux(const string &from_file,
                                                const string &to_file)
{
    input_file(file_in_, from_file);
    output_file(file_out_, to_file);

    string temp, output, s;
    size_t count = 1;
    while (std::getline(file_in_, temp))
    {
        if (file_in_.fail())
        {
            break;
        }

        size_t start = temp.find("->") + 2; // 找到产生式的起始位置
        output =
            temp.substr(0, start); // 获取左侧非终结符和箭头部分

        size_t index = temp.find('|', start);
        while (index != string::npos)
        {
            s = temp.substr(start, index - start); // 获取当前产生式
            file_out_ << count++ << " " << output << s << endl; // 输出产生式
            start = index + 1;             // 更新 start 变量
            index = temp.find('|', start); // 查找下一个 |
        }

        // 处理最后一个产生式
        s = temp.substr(start);
        if (!s.empty()) // 确保不输出空行
        {
            file_out_ << count++ << " " << output << s << endl;
        }
    }
    file_in_.close();
    file_out_.close();
}

void CFG::ContextFreeGrammar::generate_productions(const string &from_file)
{
    input_file(file_in_, from_file);

    // 读入的一行 temp
    string temp, left, right, s;

    // 获取productions
    while (std::getline(file_in_, temp))
    {
        if (file_in_.fail())
        {
            break;
        }
        // 去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");
        // 按 -> 位置分出产生式 左部 和 右部
        auto split = temp.find("->");
        left = temp.substr(0, split);
        right = temp.substr(split + 2);
        // 去除多余的空白
        remove_front_back_blank(left);
        remove_front_back_blank(right);

        auto iter = productions_.find(left);
        if (iter != productions_.end())
        {
            iter->second.emplace_back(right);
        }
        else
        {
            productions_[left].emplace_back(right);
        }
    }

    file_in_.close();
}

void CFG::ContextFreeGrammar::cal_Vn_Vt_aux(const string &from_file,
                                            const string &to_file)
{
    input_file(file_in_, from_file);
    output_file(file_out_, to_file);

    string temp, left, right;

    while (std::getline(file_in_, temp))
    {
        if (file_in_.fail())
        {
            break;
        }
        // 去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");
        size_t index = temp.find("->");
        left = temp.substr(0, index);
        right = temp.substr(index + 2);
        remove_front_back_blank(left);
        remove_front_back_blank(right);
        // 根据上下文无关文法的定义，产生式的左部 一定是非终结符
        if (std::find(non_terminal_set_.begin(), non_terminal_set_.end(),
                      left) == non_terminal_set_.end()) // 是否已经存在
        {
            non_terminal_set_.push_back(left);
        }
        // 文法中的符号决定集合成员；类型表只负责输出标签。
        // 流提取跳过连续空白，避免把空字符串当作符号后访问 s[0]。
        std::istringstream input_symbols(right);
        set<string> symbols;
        string symbol;
        while (input_symbols >> symbol)
        {
            symbols.insert(symbol);
        }
        // 保持旧实现每行按字典序收集符号的顺序，避免引入无关顺序变化。
        for (const auto& symbol : symbols)
        {
            // generate_productions 已读完整份文法；出现在产生式左部的符号
            // 就是非终结符。这样也能识别 <A>'、<A>'' 和后面才定义的规则。
            const bool non_terminal = productions_.find(symbol) != productions_.end();
            if (non_terminal)
            {
                append_unique(non_terminal_set_, symbol);
            }
            else
            {
                append_unique(terminal_set_, symbol);
            }
        }
    }

    // 每行符号先由 set 排序去重，再按首次出现顺序加入 vector；整体并非全局升序。
    // 输出非终结符集合
    file_out_ << "非终结符号集合 Vn : " << endl;
    for (const auto &non_t : non_terminal_set_)
    {
        file_out_ << non_t << endl;
    }
    // 输出终结符集合
    file_out_ << "终结符号集合 Vt : " << endl;
    for (const auto &t : terminal_set_)
    {
        const auto type = terminal_type_.find(t);
        const string label = type == terminal_type_.end()
            ? "文法终结符（未登记类型）" : type->second;
        file_out_ << std::setiosflags(ios::left) << std::setw(10) << t << " "
                  << std::setw(10) << label << endl;
    }

    file_in_.close();
    file_out_.close();
}

// 辅助函数，用于递归计算 FIRST 集
void CFG::ContextFreeGrammar::_cal_first_aux(const string &symbol)
{
    if (!first_[symbol].empty())
    {
        return; // 已经计算过，避免重复计算
    }

    auto it = productions_.find(symbol);
    if (it == productions_.end())
        return; // 没有找到产生式

    for (const auto &production : it->second)
    {
        std::vector<std::string> symbols = split_string(production);
        if (symbols.empty())
            continue; // 如果产生式为空，跳过

        bool canDeriveEmpty = true; // 标记是否可以推导出空串

        for (const auto &sym : symbols)
        {
            if (is_non_terminal(sym))
            { // 是非终结符
                _cal_first_aux(sym);
                // 处理当前非终结符的 FIRST 集合
                bool foundEmpty = false;
                for (const auto &firstSymbol : first_[sym])
                {
                    if (firstSymbol == "empty")
                    {
                        foundEmpty = true; // 标记可以推导出空串
                    }
                    else if (std::find(first_[symbol].begin(),
                                         first_[symbol].end(),
                                         firstSymbol) == first_[symbol].end())
                    {
                        first_[symbol].push_back(firstSymbol);
                    }
                }
                // 如果当前非终结符没有空串，则停止后续处理
                if (!foundEmpty)
                {
                    canDeriveEmpty = false;
                    break;
                }
            }
            else
            { // 是终结符
                if (std::find(first_[symbol].begin(), first_[symbol].end(),
                              sym) == first_[symbol].end())
                {
                    first_[symbol].push_back(sym);
                }
                canDeriveEmpty = false; // 找到终结符，不考虑空串
                break;                  // 找到一个终结符，结束这个产生式的处理
            }
        }

        // 如果可以推导出空串，则在集合中加入 "empty"
        if (canDeriveEmpty)
        {
            if (std::find(first_[symbol].begin(), first_[symbol].end(),
                          "empty") == first_[symbol].end())
            {
                first_[symbol].emplace_back("empty");
            }
        }
    }
}

void CFG::ContextFreeGrammar::cal_first_aux()
{
    // 具体解题思路：求解文法中所有的产生式左部的FIRST集合，只需要看每个左部的所有产生式右部的首字符A。
    // 1. 如果A是终结符，那么将其加入到左部对应的
    // 集合中。
    // 2.
    // 如果A是非终结符，那么继续求解，并将加入到左部对应的集合中去。
    // 3. 如果，那么也将加入其中。
    // 注意：如果产生式的右部由“|”连接，对于"|"左右两边的句型运用上述相同的规则。

    // 对于终结符号，FIRST 集合为自身
    for (const auto &terminal : terminal_set_)
    {
        first_[terminal].push_back(terminal);
    }

    // 处理非终结符号
    for (const auto &non_terminal : non_terminal_set_)
    {
        _cal_first_aux(non_terminal);
    }

    // 输出非终结符号的 FIRST 集合
    file_out_ << "非终结符号的 FIRST 集合：" << endl;
    for (const auto &non_terminal : non_terminal_set_)
    {
        file_out_ << std::setiosflags(ios::left) << std::setw(30)
                  << non_terminal << "的 FIRST 集合为：";
        for (const auto &f : first_[non_terminal])
        {
            file_out_ << std::setiosflags(std::ios::left) << std::setw(20) << f;
        }
        file_out_ << endl;
    }
}

CFG::ContextFreeGrammar::SequenceFirst
CFG::ContextFreeGrammar::first_of_sequence(
    const std::vector<std::string>& symbols, std::size_t begin) const
{
    if (begin > symbols.size())
    {
        throw std::out_of_range("FIRST sequence offset exceeds sequence size");
    }

    SequenceFirst result;
    for (std::size_t i = begin; i < symbols.size(); ++i)
    {
        const auto first_it = first_.find(symbols[i]);
        if (first_it == first_.end())
        {
            throw std::logic_error("Missing FIRST set for: " + symbols[i]);
        }

        bool symbol_nullable = false;
        for (const auto& value : first_it->second)
        {
            if (value == "empty")
            {
                symbol_nullable = true;
            }
            else
            {
                append_unique(result.terminals, value);
            }
        }
        if (!symbol_nullable)
        {
            result.nullable = false;
            return result;
        }
    }
    return result;
}

void CFG::ContextFreeGrammar::cal_follow_aux()
{
    follow_.clear();
    for (const auto& non_terminal : non_terminal_set_)
    {
        follow_.try_emplace(non_terminal);
    }
    // 当前项目约定 <S> 为开始符号。
    follow_.at("<S>").push_back("$");

    bool changed;
    do
    {
        changed = false;
        for (const auto& A : non_terminal_set_)
        {
            const auto production_it = productions_.find(A);
            if (production_it == productions_.end())
            {
                throw std::logic_error("Missing productions for: " + A);
            }
            for (const auto& right : production_it->second)
            {
                const auto symbols = split_string(right);
                for (std::size_t i = 0; i < symbols.size(); ++i)
                {
                    const auto& B = symbols[i];
                    if (!is_non_terminal(B))
                    {
                        continue;
                    }

                    const auto suffix = first_of_sequence(symbols, i + 1);
                    auto& target = follow_.at(B);
                    for (const auto& terminal : suffix.terminals)
                    {
                        if (append_unique(target, terminal))
                        {
                            changed = true;
                        }
                    }
                    if (suffix.nullable)
                    {
                        // A 与 B 相同时不会新增元素，集合已包含自身。
                        for (const auto& terminal : follow_.at(A))
                        {
                            if (append_unique(target, terminal))
                            {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    } while (changed);

    file_out_ << "非终结符号的 FOLLOW 集合：" << endl;
    for (const auto &non_terminal : non_terminal_set_)
    {
        file_out_ << std::setiosflags(std::ios::left) << std::setw(30)
                  << non_terminal << "的 FOLLOW 集合为：";
        for (const auto &f : follow_[non_terminal])
        {
            file_out_ << std::setiosflags(std::ios::left) << std::setw(20) << f;
        }
        file_out_ << endl;
    }
}

void CFG::ContextFreeGrammar::generate_predictive_analysis_table_aux()
{
    /*
     * 方法：对于文法中每个产生式 A->α ，进行如下处理：
     * 1）对于 FIRST(α)中的每个终结符号 a，将 A->α 加入到 M[A, a]
     * 中。
     * 2）如果 empty 在 FIRST(α) 中，那么对 FOLLOW(A)中的每个终结符号 b，将
     * A->α 加入到 M[A, b] 中。
     * 如果 empty 在 FIRST(α) 中，且 $在 FOLLOW(A) 中，也将 A->α 加入到
     * M[A,$] 中。
     *
     * 在完成以上操作以后，如果 M[A, a] 中没有产生式，那么将 M[A, a] 设置为
     * error（我们通常在表中用一个空条目表示）
     */

    file_out_ << "非终结符号的数量：" << non_terminal_set_.size() << endl;
    string str = " + $ ";
    file_out_ << "终结符号的数量：" << terminal_set_.size() << str << endl;

    string temp;

    map<string, int> num_prod;

    while (std::getline(file_in_, temp))
    {
        if (file_in_.fail())
        {
            break;
        }
        int num = std::atoi(temp.substr(0, temp.find(' ')).c_str());
        // 去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");

        num_prod.insert(std::make_pair(temp, num));

        size_t split = temp.find("->");
        string left = temp.substr(0, split);
        string right = temp.substr(split + 2);
        remove_front_back_blank(left);
        remove_front_back_blank(right);
        vector<string> strings = split_string(right);

        // 计算整个右部序列的 FIRST；不能把每个符号的 FIRST 直接取并集。
        const auto sequence = first_of_sequence(strings, 0);
        for (const auto& terminal : sequence.terminals)
        {
            append_unique(predictive_analysis_table_.table_[left][terminal], temp);
        }
        if (sequence.nullable)
        {
            // 只有右部所有符号都能推出空串时，才使用左部的 FOLLOW。
            for (const auto& terminal : follow_.at(left))
            {
                append_unique(predictive_analysis_table_.table_[left][terminal], temp);
            }
        }
    }

    // 去除推出 empty 的产生式
    for (const auto &non : non_terminal_set_)
    {
        for (const auto &ter : terminal_set_)
        {
            auto &vec =
                predictive_analysis_table_
                    .table_[non][ter]; // 确保对vec的引用，以便修改
            for (auto iter = vec.begin(); iter != vec.end();)
            {
                size_t index = iter->find("->");
                if (index != std::string::npos)
                {
                    string right = iter->substr(index + 2);
                    remove_front_back_blank(right);
                    vector<string> split = split_string(right);

                    if (!split.empty() && split[0] == "empty")
                    {
                        iter = vec.erase(iter); // 移除产生式
                    }
                    else
                    {
                        ++iter; // 增加迭代器
                    }
                }
                else
                {
                    ++iter; // 如果没有找到 "->"，增加迭代器
                }
            }
        }
    }

    // 对于 grammar.txt 文件中具有二义性的非终结符号特殊处理，以消除二义性

    // 对于 <factor> 推出的产生式
    for (const auto &ter : terminal_set_)
    {
        auto &vec = predictive_analysis_table_.table_["<factor>"][ter];
        if (vec.size() == 2)
        {
            // 去除能推出两个 产生式 中的 101 号产生式
            std::swap(vec[0], vec[1]);
            vec.pop_back();
        }
    }

    // 对于 <expression> 产生式
    for (const auto &ter : terminal_set_)
    {
        auto &vec = predictive_analysis_table_.table_["<expression>"][ter];
        if (vec.size() == 2)
        {
            if (ter == "identifier1")
            {
                // 保留 68 <expression> -> <var1> ....
                vec.pop_back();
            }
            else // ter == "identifier" || ter == "(" || ter == "number" ||
                   // ter == "identifier
            {
                std::swap(vec[0], vec[1]);
                vec.pop_back();
            }
        }
    }

    // 对于推出 ; 的产生式
    for (const auto &non : non_terminal_set_)
    {
        auto &vec = predictive_analysis_table_.table_[non][";"];
        if (vec.size() == 2)
        {
            // 保留 能直接推出 ; 的产生式
            vec.pop_back();
        }
    }

    // 保留旧 Parser 对显式 empty 的回退约定。
    // 先归并待移除元素再 erase，避免遍历过程中迭代器失效。
    for (const auto& non : non_terminal_set_)
    {
        auto& choices = predictive_analysis_table_.table_[non]["$"];
        choices.erase(std::remove_if(choices.begin(), choices.end(),
            [](const string& production)
            {
                const auto arrow = production.find("->");
                if (arrow == string::npos) return false;
                const auto symbols = split_string(production.substr(arrow + 2));
                return !symbols.empty() && symbols[0] == "empty";
            }), choices.end());
    }

    predictive_analysis_table_.Vn_ = non_terminal_set_;
    predictive_analysis_table_.Vt_ = terminal_set_;
    predictive_analysis_table_.productions_ = productions_;

    // 格式化输出到文件中
    file_out_ << std::setiosflags(ios::right) << std::setw(54)
              << "输入的终结符号" << endl;
    file_out_ << "非终结符号" << std::setw(21) << " ";

    for (const auto &ter : terminal_set_)
    {
        file_out_ << std::setw(20) << ter;
    }
    file_out_ << std::setw(20) << "$" << endl;
    for (const auto &non : non_terminal_set_)
    {
        file_out_ << std::setiosflags(ios::right) << non
                  << std::setw(30 - static_cast<int>(non.size())) << " ";
        for (const auto &ter : terminal_set_)
        {
            auto strings = predictive_analysis_table_.table_[non][ter];
            if (strings.empty())
            {
                file_out_ << std::setiosflags(ios::right) << std::setw(20)
                          << "error";
            }
            else
            {
                vector<int> nums;
                for (const auto &s1 : strings)
                {
                    auto iter = num_prod.find(s1);
                    if (iter != num_prod.end())
                    {
                        nums.emplace_back(iter->second);
                    }
                }
                if (nums.size() == 1)
                {
                    file_out_ << std::setiosflags(ios::right) << std::setw(20)
                              << nums[0];
                }
                else
                {
                    string output;
                    for (auto i = 0; i < nums.size(); i++)
                    {
                        output += std::to_string(nums[i]);
                        if (i < (nums.size() - 1))
                        {
                            output += ",";
                        }
                    }
                    file_out_ << std::setiosflags(ios::right) << std::setw(20)
                              << output;
                }
            }
        }
        // 输出 non, $ 对应的产生式序号
        auto strings = predictive_analysis_table_.table_[non]["$"];
        if (strings.empty())
        {
            file_out_ << std::setiosflags(ios::right) << std::setw(20)
                      << "error" << endl;
        }
        else
        {
            vector<int> nums;
            for (const auto &s1 : strings)
            {
                auto iter = num_prod.find(s1);
                if (iter != num_prod.end())
                {
                    nums.emplace_back(iter->second);
                }
            }
            if (nums.size() == 1)
            {
                file_out_ << std::setiosflags(ios::right) << std::setw(20)
                          << nums[0] << endl;
            }
            else
            {
                string output;
                for (auto i = 0; i < nums.size(); i++)
                {
                    output += std::to_string(nums[i]);
                    if (i < (nums.size() - 1))
                    {
                        output += ",";
                    }
                }
                file_out_ << std::setiosflags(ios::right) << std::setw(20)
                          << output << endl;
            }
        }
    }
}
