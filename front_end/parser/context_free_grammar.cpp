//
// Created by 86198 on 2024/11/1.
//

#include "context_free_grammar.h"


// 函数：查找公共前缀
string findLongestCommonPrefix(const vector<string>& strings)
{
    if (strings.empty()) return "";

    string prefix = strings[0];  // 以第一个字符串为基础进行比较

    for (size_t i = 1; i < strings.size(); ++i)
    {
        while (strings[i].find(prefix) != 0)
        { // 当当前字符串不以 prefix 开头时
            prefix = prefix.substr(0, prefix.size() - 1); // 缩短 prefix
            if (prefix.empty()) return ""; // 如果 prefix 为空，返回空
        }
    }
    if(prefix[0] == '<' && prefix.size() == 1)
    {
        return "";
    }

    return prefix;
}




void CFG::ContextFreeGrammar::remove_direct_left_recursion_and_factor_aux
    (const string &from_file, const string &to_file)
{

    input_file(file_in_, from_file);
    output_file(file_out_, to_file);


    //读入的一行 temp
    string temp, part1, part2, p2_1, p2_2, s;
    size_t split, index;
    while(std::getline(file_in_, temp))
    {
        if(file_in_.fail())
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
            remove_front_back_blank(part2);            //将每一行语法分为箭头前后的两个部分
            index = part2.find('|');
            p2_1.assign(part2, 0, index);
            p2_2.assign(part2, index + 1, part2.length() - index);
            remove_front_back_blank(p2_1);
            remove_front_back_blank(p2_2);            //将part2部分按照|前后分为两部分
            if (p2_1.find(part1) == 0 && p2_1[part1.length()] == ' ')
            {
                p2_1.assign(p2_1, part1.length() + 1, p2_1.length() - part1.length());
                file_out_ << part1 << " -> ";
                if (p2_2 != "empty")
                    file_out_ << p2_2 << " ";
                else
                    file_out_ << p2_2 << " | ";
                file_out_ << part1 << "'" << endl;
                file_out_ << part1 << "' -> " << p2_1 << " " << part1 << "' | empty" << endl;
            }            //p2_1出现part1说明出现了左递归
            else
            {
                vector<string> strings_;
                size_t start= temp.find("->") + 2;
                //对右部 按照 | 进行分隔
                size_t index_ = temp.find('|', start);
                while (index_ != string::npos)
                {
                    s = temp.substr(start, index_ - start); // 获取当前产生式
                    remove_front_back_blank(s);
                    strings_.push_back(s);
                    start = index_ + 1; // 更新 start 变量
                    index_ = temp.find('|', start); // 查找下一个 |
                }

                // 处理最后一个产生式
                s = temp.substr(start);
                remove_front_back_blank(s);
                strings_.push_back(s);

                //查找公共前缀因子
                string common = findLongestCommonPrefix(strings_);
                remove_front_back_blank(common);

                if(common.empty())
                {
                    file_out_ << temp << endl;
                }
                else
                {
                    string part1_ = part1 + "'";
                    file_out_ << part1 << " -> " << common << " " << part1_ << endl;
                    file_out_ << part1_ << " -> ";
                    for (const string& str : strings_)
                    {
                        string nonCommonPrefix = str.substr(common.size());
                        remove_front_back_blank(nonCommonPrefix);
                        if(str == strings_.front())
                        {
                            if(nonCommonPrefix.empty())
                            {
                                file_out_ << "empty";
                            }
                            else
                            {
                                file_out_ << nonCommonPrefix;
                            }
                        }
                        else
                        {
                            if(nonCommonPrefix.empty())
                            {
                                file_out_ << " | empty";
                            }
                            else
                            {
                                file_out_ << " | " << nonCommonPrefix;
                            }
                        }
                    }
                    file_out_ << endl;
                }
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


void CFG::ContextFreeGrammar::split_grammar_aux(const string &from_file, const string &to_file)
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
        output = temp.substr(0, start); // 获取左侧非终结符和箭头部分

        size_t index = temp.find('|', start);
        while (index != string::npos)
        {
            s = temp.substr(start, index - start); // 获取当前产生式
            file_out_ << count++ << " " << output << s << endl; // 输出产生式
            start = index + 1; // 更新 start 变量
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


    //读入的一行 temp
    string temp, left, right, s;

    //获取productions
    while(std::getline(file_in_, temp))
    {
        if(file_in_.fail())
        {
            break;
        }
        //去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");
        //按 -> 位置分出产生式 左部 和 右部
        auto split = temp.find("->");
        left = temp.substr(0, split);
        right = temp.substr(split + 2);
        //去除多余的空白
        remove_front_back_blank(left);
        remove_front_back_blank(right);

        auto iter = productions_.find(left);
        if(iter != productions_.end())
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


void CFG::ContextFreeGrammar::cal_Vn_Vt_aux(const string &from_file, const string &to_file)
{
    input_file(file_in_, from_file);
    output_file(file_out_, to_file);


    string temp, left, right;

    while(std::getline(file_in_, temp))
    {
        if(file_in_.fail())
        {
            break;
        }
        //去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");
        size_t index = temp.find("->");
        left = temp.substr(0, index);
        right = temp.substr(index + 2);
        remove_front_back_blank(left);
        remove_front_back_blank(right);
        //根据上下文无关文法的定义，产生式的左部 一定是非终结符
        if(std::find(non_terminal_set_.begin(), non_terminal_set_.end(), left)== non_terminal_set_.end())//是否已经存在
        {
            non_terminal_set_.push_back(left);
        }
        set<string> strings;
        //按照 ‘ ’ 划分 right
        size_t start = 0;
        size_t index_ = right.find(' ', start);
        while(index_ != string::npos)
        {
            string t = right.substr(start, index_ - start);
            remove_front_back_blank(t);
            strings.insert(t);
            start = index_ + 1;
            index_ = right.find(' ', start);
        }
        string last = right.substr(start);
        remove_front_back_blank(last);
        strings.insert(last);
        for(const auto& s : strings)
        {
            if(std::find(non_terminal_set_.begin(), non_terminal_set_.end(), s) == non_terminal_set_.end())
            {
                if(s[0] == '<' && s[s.length() - 1] == '>')
                {
                    non_terminal_set_.push_back(s);
                }
                else
                {
                    if(std::find(terminal_set_.begin(), terminal_set_.end(), s) == terminal_set_.end() &&
                       terminal_type_.find(s) != terminal_type_.end())
                    {
                        terminal_set_.push_back(s);
                    }
                }
            }
        }
    }

    //鉴于非终结符以及终结符集合的数据结构是 set，故输出是升序输出
    //输出非终结符集合
    file_out_ << "非终结符号集合 Vn : " << endl;
    for(const auto& non_t : non_terminal_set_)
    {
        file_out_ << non_t << endl;
    }
    //输出终结符集合
    file_out_ << "终结符号集合 Vt : " << endl;
    for(const auto& t : terminal_set_)
    {
        file_out_ << std::setiosflags(ios::left) << std::setw(10) << t << " " << std::setw(10) << terminal_type_.find(t)->second << endl;
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

    for (const auto& production : it->second)
    {
        std::vector<std::string> symbols = split_string(production);
        if (symbols.empty()) continue; // 如果产生式为空，跳过

        bool canDeriveEmpty = true; // 标记是否可以推导出空串

        for (const auto& sym : symbols)
        {
            if (is_non_terminal(sym))
            { // 是非终结符
                _cal_first_aux(sym);
                // 处理当前非终结符的 FIRST 集合
                bool foundEmpty = false;
                for (const auto& firstSymbol : first_[sym])
                {
                    if (firstSymbol == "empty")
                    {
                        foundEmpty = true; // 标记可以推导出空串
                    }
                    else if (std::find(first_[symbol].begin(), first_[symbol].end(), firstSymbol) == first_[symbol].end())
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
                if (std::find(first_[symbol].begin(), first_[symbol].end(), sym) == first_[symbol].end())
                {
                    first_[symbol].push_back(sym);
                }
                canDeriveEmpty = false; // 找到终结符，不考虑空串
                break; // 找到一个终结符，结束这个产生式的处理
            }
        }

        // 如果可以推导出空串，则在集合中加入 "empty"
        if (canDeriveEmpty) {
            if (std::find(first_[symbol].begin(), first_[symbol].end(), "empty") == first_[symbol].end()) {
                first_[symbol].emplace_back("empty");
            }
        }
    }
}



void CFG::ContextFreeGrammar::cal_first_aux()
{
    //具体解题思路：求解文法中所有的产生式左部的FIRST集合，只需要看每个左部的所有产生式右部的首字符A。
    //1. 如果A是终结符，那么将其加入到左部对应的 集合中。
    //2. 如果A是非终结符，那么继续求解，并将加入到左部对应的集合中去。
    //3. 如果，那么也将加入其中。
    //注意：如果产生式的右部由“|”连接，对于"|"左右两边的句型运用上述相同的规则。

    // 对于终结符号，FIRST 集合为自身
    for (const auto& terminal : terminal_set_)
    {
        first_[terminal].push_back(terminal);
    }

    // 处理非终结符号
    for (const auto& non_terminal : non_terminal_set_)
    {
        _cal_first_aux(non_terminal);
    }

    // 输出非终结符号的 FIRST 集合
    file_out_ << "非终结符号的 FIRST 集合：" << endl;
    for (const auto& non_terminal : non_terminal_set_)
    {
        file_out_ << std::setiosflags(ios::left) << std::setw(30) << non_terminal << "的 FIRST 集合为：";
        for (const auto& f : first_[non_terminal])
        {
            file_out_ << std::setiosflags(std::ios::left) << std::setw(20) << f;
        }
        file_out_ << endl;
    }
}


void CFG::ContextFreeGrammar::cal_follow_aux()
{
    // 1. 首先将特殊符号 "$" 加入开始符号 S 的 FOLLOW 集合中
    follow_["<S>"].emplace_back("$");

    // 2. 列举出所有能够推导出 empty 的非终结符
    std::unordered_set<std::string> empty_non;
    for (const auto& non : non_terminal_set_)
    {
        auto it = productions_.find(non);
        if (it != productions_.end())
        {
            const auto& right = it->second;
            if (std::find(right.begin(), right.end(), "empty") != right.end())
            {
                empty_non.insert(non);
            }
        }
    }

    // 3. 对于每个非终结符 A
    bool changed;
    do
    {
        changed = false;

        for (const auto& A : non_terminal_set_)
        {
            auto it = productions_.find(A);
            if (it == productions_.end())
            {
                std::cerr << "未找到非终结符 " << A << " 的产生式。" << std::endl;
                continue;
            }

            // 检查 A 的所有产生式
            for (const auto& right : it->second)
            {
                std::vector<std::string> strings = split_string(right);
                for (size_t i = 0; i < strings.size(); ++i)
                {
                    const std::string& symbol = strings[i];
                    if (is_non_terminal(symbol)) // 存在 B
                    {
                        // 如果是 A -> αB
                        if (i + 1 == strings.size())
                        {
                            // 将 FOLLOW(A) 加入到 FOLLOW(B) 中
                            for (const auto& followSymbol : follow_[A])
                            {
                                // 检查并添加符号到 FOLLOW(B)
                                if (std::find(follow_[symbol].begin(), follow_[symbol].end(), followSymbol) == follow_[symbol].end())
                                {
                                    follow_[symbol].emplace_back(followSymbol);
                                    changed = true;
                                }
                            }
                        }
                        else // 对应于 A -> αBβ
                        {
                            const std::string& beta = strings[i + 1];

                            if (is_terminal(beta)) // β 是终结符
                            {
                                // 直接将 β 添加到 FOLLOW(B)
                                if (std::find(follow_[symbol].begin(), follow_[symbol].end(), beta) == follow_[symbol].end())
                                {
                                    follow_[symbol].emplace_back(beta);
                                    changed = true;
                                }
                            }
                            else // β 是非终结符
                            {
                                // 将 FIRST(β) 中除了 empty 之外的符号加入到 FOLLOW(B) 中
                                for (const auto& firstSymbol : first_[beta])
                                {
                                    if (firstSymbol != "empty" && std::find(follow_[symbol].begin(), follow_[symbol].end(), firstSymbol) == follow_[symbol].end())
                                    {
                                        follow_[symbol].emplace_back(firstSymbol);
                                        changed = true;
                                    }
                                }

                                // 如果 FIRST(β) 中包含 empty，则要将 FOLLOW(A) 加入到 FOLLOW(B) 中
                                if (empty_non.find(beta) != empty_non.end())
                                {
                                    for (const auto& followSymbol : follow_[A])
                                    {
                                        if (std::find(follow_[symbol].begin(), follow_[symbol].end(), followSymbol) == follow_[symbol].end())
                                        {
                                            follow_[symbol].emplace_back(followSymbol);
                                            changed = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    } while (changed); // 如果有变更继续循环

    // 将 FOLLOW 集合输出到文件中
    file_out_ << "非终结符号的 FOLLOW 集合：" << endl;
    for (const auto& non_terminal : non_terminal_set_)
    {
        file_out_ << std::setiosflags(std::ios::left) << std::setw(30) << non_terminal << "的 FOLLOW 集合为：";
        for (const auto& f : follow_[non_terminal])
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
     * 1）对于 FIRST(α)中的每个终结符号 a，将 A->α 加入到 M[A, a] 中。
     * 2）如果 empty 在 FIRST(α) 中，那么对 FOLLOW(A)中的每个终结符号 b，将 A->α 加入到 M[A, b] 中。
     * 如果 empty 在 FIRST(α) 中，且 $在 FOLLOW(A) 中，也将 A->α 加入到 M[A,$] 中。
     *
     * 在完成以上操作以后，如果 M[A, a] 中没有产生式，那么将 M[A, a] 设置为 error（我们通常在表中用一个空条目表示）
     */

    file_out_ << "非终结符号的数量：" << non_terminal_set_.size() << endl;
    string str = " + $ ";
    file_out_ << "终结符号的数量：" << terminal_set_.size() << str << endl;

    string temp;

    map<string, int> num_prod;

    while(std::getline(file_in_, temp))
    {
        if(file_in_.fail())
        {
            break;
        }
        int num = std::atoi(temp.substr(0, temp.find(' ')).c_str());
        //去掉每行开头的数字
        temp.replace(0, temp.find(' '), "");

        num_prod.insert(std::make_pair(temp, num));

        size_t split = temp.find("->");
        string left = temp.substr(0, split);
        string right = temp.substr(split + 2);
        remove_front_back_blank(left);
        remove_front_back_blank(right);
        vector<string> strings = split_string(right);

        for(const auto& s : strings)
        {
            for(const auto& a : first_[s])
            {
                //1）对于 FIRST(α)中的每个终结符号 a，将 A->α 加入到 M[A, a] 中。
                if(std::find(predictive_analysis_table_.table_[left][a].begin(), predictive_analysis_table_.table_[left][a].end(), temp)
                    == predictive_analysis_table_.table_[left][a].end())   //不重复才加入
                {
                    predictive_analysis_table_.table_[left][a].emplace_back(temp);
                }
                if(a == "empty")    // 2）如果 empty 在 FIRST(α) 中
                {
                    for(const auto& b : follow_[left])
                    {
                        //那么对 FOLLOW(A)中的每个终结符号 b，将 A->α 加入到 M[A, b] 中。
                        if(std::find(predictive_analysis_table_.table_[left][b].begin(), predictive_analysis_table_.table_[left][b].end(), temp)
                           == predictive_analysis_table_.table_[left][b].end())
                        {
                            predictive_analysis_table_.table_[left][b].emplace_back(temp);
                        }
                        if(b == "$")    //且 $在 FOLLOW(A) 中，也将 A->α 加入到 M[A,$] 中。
                        {
                            if(std::find(predictive_analysis_table_.table_[left]["$"].begin(), predictive_analysis_table_.table_[left]["$"].end(), temp)
                               == predictive_analysis_table_.table_[left]["$"].end())
                            {
                                predictive_analysis_table_.table_[left]["$"].emplace_back(temp);
                            }
                        }
                    }
                }
            }
        }
    }


    // 去除推出 empty 的产生式
    for (const auto& non : non_terminal_set_)
    {
        for (const auto& ter : terminal_set_)
        {
            auto& vec = predictive_analysis_table_.table_[non][ter]; // 确保对vec的引用，以便修改
            for (auto iter = vec.begin(); iter != vec.end(); )
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


    //对于 grammar.txt 文件中具有二义性的非终结符号特殊处理，以消除二义性

    //对于 <factor> 推出的产生式
    for(const auto& ter : terminal_set_)
    {
        auto& vec = predictive_analysis_table_.table_["<factor>"][ter];
        if(vec.size() == 2)
        {
            //去除能推出两个 产生式 中的 101 号产生式
            std::swap(vec[0], vec[1]);
            vec.pop_back();
        }
    }


    //对于 <expression> 产生式
    for(const auto& ter : terminal_set_)
    {
        auto& vec = predictive_analysis_table_.table_["<expression>"][ter];
        if(vec.size() == 2)
        {
            if(ter == "identifier1")
            {
                //保留 68 <expression> -> <var1> ....
                vec.pop_back();
            }
            else //ter == "identifier" || ter == "(" || ter == "number" || ter == "identifier
            {
                std::swap(vec[0], vec[1]);
                vec.pop_back();
            }
        }
    }

    //对于推出 ; 的产生式
    for(const auto& non : non_terminal_set_)
    {
        auto& vec = predictive_analysis_table_.table_[non][";"];
        if(vec.size() == 2)
        {
            //保留 能直接推出 ; 的产生式
            vec.pop_back();
        }
    }


    for(const auto& non : non_terminal_set_)
    {
        for(const auto& s : predictive_analysis_table_.table_[non]["$"])
        {
            auto& vec = predictive_analysis_table_.table_[non]["$"]; // 确保对vec的引用，以便修改
            for (auto iter = vec.begin(); iter != vec.end(); )
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


    predictive_analysis_table_.Vn_ = non_terminal_set_;
    predictive_analysis_table_.Vt_ = terminal_set_;
    predictive_analysis_table_.productions_ = productions_;


    //格式化输出到文件中
    file_out_ << std::setiosflags(ios::right) << std::setw(54) << "输入的终结符号" <<endl;
    file_out_ << "非终结符号" << std::setw(21) << " ";

    for(const auto& ter : terminal_set_)
    {
        file_out_ << std::setw(20) << ter;
    }
    file_out_ << std::setw(20) << "$" << endl;
    for(const auto& non : non_terminal_set_)
    {
        file_out_ << std::setiosflags(ios::right) << non << std::setw(30 - static_cast<int>(non.size())) << " ";
        for(const auto& ter : terminal_set_)
        {
            auto strings = predictive_analysis_table_.table_[non][ter];
            if(strings.empty())
            {
                file_out_ << std::setiosflags(ios::right) << std::setw(20) << "error";
            }
            else
            {
                vector<int> nums;
                for(const auto& s1 : strings)
                {
                    auto iter = num_prod.find(s1);
                    if(iter != num_prod.end())
                    {
                        nums.emplace_back(iter->second);
                    }
                }
                if(nums.size() == 1)
                {
                    file_out_ << std::setiosflags(ios::right) << std::setw(20) << nums[0];
                }
                else
                {
                    string output;
                    for(auto i = 0;i < nums.size();i++)
                    {
                        output += std::to_string(nums[i]);
                        if(i < (nums.size() - 1))
                        {
                            output += ",";
                        }
                    }
                    file_out_ << std::setiosflags(ios::right) << std::setw(20) << output;
                }
            }
        }
        //输出 non, $ 对应的产生式序号
        auto strings = predictive_analysis_table_.table_[non]["$"];
        if(strings.empty())
        {
            file_out_ << std::setiosflags(ios::right) << std::setw(20) << "error" << endl;
        }
        else
        {
            vector<int> nums;
            for(const auto& s1 : strings)
            {
                auto iter = num_prod.find(s1);
                if(iter != num_prod.end())
                {
                    nums.emplace_back(iter->second);
                }
            }
            if(nums.size() == 1)
            {
                file_out_ << std::setiosflags(ios::right) << std::setw(20) << nums[0] << endl;
            }
            else
            {
                string output;
                for(auto i = 0;i < nums.size();i++)
                {
                    output += std::to_string(nums[i]);
                    if(i < (nums.size() - 1))
                    {
                        output += ",";
                    }
                }
                file_out_ << std::setiosflags(ios::right) << std::setw(20) << output << endl;
            }
        }
    }
}





