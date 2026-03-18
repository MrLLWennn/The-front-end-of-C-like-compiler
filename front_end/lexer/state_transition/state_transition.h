//
// Created by 86198 on 2024/10/21.
//

#ifndef MY_COMPILER_STATE_TRANSITION_H
#define MY_COMPILER_STATE_TRANSITION_H


#include <utility>

#include"D:\clion\my_compiler\token\token.h"


//此头文件用于描述词法生成器的其中一项功能，即RE->NFA->DFA->minimized DFA


namespace RE_NFA_DFA
{


    //NFA和DFA状态转移图中的结点
//    class Node
//    {
//    private:
//        //helper function
//        static void print_node(Node* node);
//
//        static void print_children(Node* node);
//
//        friend std::ostream& operator<<(std::ostream&, const Node&);
//
//
//    public:
//
//        static Node* of(const string& label, const int& level);
//
//        static void destroy(Node* node);
//
//        void append_child(Node* node);
//
//        void append_token(const my_compiler_token::Token& token);
//
//        static void print(Node* node);
//
//        vector<my_compiler_token::Token> get_identifier_tokens();
//
//    public:
//        vector<Node*> children_;
//        string label_;
//        vector<my_compiler_token::Token> tokens_;
//        int level_;
//    };


    //状态结点
    class State
    {
    private:
        string name_;
        //开始状态
        bool starting_;
        //接收状态
        bool accepting_;
        std::multimap<char, State*> transitions_;

    public:
        explicit State(string name, bool starting = false, bool accepting = false)
        :name_(std::move(name)), starting_(starting), accepting_(accepting)
        {
        }

        State()
        :starting_(false), accepting_(false)
        {
        }

    public:
        [[nodiscard("no use return")]] bool is_starting() const noexcept
        {
            return this->starting_;
        }

        [[nodiscard("no use return")]] bool is_accepting() const noexcept
        {
            return this->accepting_;
        }

    public:
        void add_transition(const char& sym, State*& to)
        {
            add_transition_aux(sym, to);
        }

        std::pair<std::multimap<char, State *>::iterator, std::multimap<char, State *>::iterator> get_transition(const char& sym) // first->second
        {
            auto all_transitions = transitions_.equal_range(sym);
            return all_transitions;
        }

        void set_starting(bool starting)
        {
            starting_ = starting;
        }

        void set_accepting(bool accepting)
        {
            accepting_ = accepting;
        }

        [[nodiscard]] const string& get_name() const
        {
            return name_;
        }

        [[nodiscard]] std::multimap<char, State*> get_all_transition() const
        {
            return transitions_;
        }

    private:
        void add_transition_aux(const char& sym, State*& to)
        {
            transitions_.insert(std::make_pair(sym, to));
        }
    };


    //非确定有限自动机
    /*
     * 一个非确定的有穷自动机是由以下及部分构成的
     *      1）一个有穷的状态集合
     *      2）一个输入符合集合，即输入字母表，假设空串不是其中的元素
     *      3）一个转换函数。对每一个状态、字母表以及空串都给出了对应的后继状态集合
     *      4）开始状态
     *      5）接收状态集合
     * */
    class NFA
    {

    private:
        //有穷的状态集合
        vector<State*> all_states_;
        //输入符号集
        set<char> alphabet_;
        //开始状态
        State* starting_state_;
        //接受状态集合
        vector<State*>accepting_states_;
        //空字符
        char epsilon_;
        //空闭包转换函数
        //功能：计算状态s的 空闭包，及通过 空 转换到达的所有状态集合
        //参数：参数一：待计算的状态s，已经判断的状态集合，防止重复访问
        set<State*> epsilon_closure_aux(State* s, set<State*> already_evaluated) const
        {
            //存放空转换到达的所有状态集合
            set<State*> return_states;
            return_states.insert(s);
            already_evaluated.insert(s);
            //获取 s 的所有 ε 转换
            auto evaluate = s->get_transition(epsilon_);
            for(auto iter = evaluate.first; iter != evaluate.second;++iter)
            {
                //跳过已经保存了的状态
                if(return_states.find(iter->second) != return_states.end() || already_evaluated.find(iter->second) != already_evaluated.end()) continue;
                set<State*> e_closure = epsilon_closure_aux(iter->second, already_evaluated);
                //插入到闭包中
                for(auto state:e_closure)
                {
                    return_states.insert(state);
                }
            }
            return return_states;
        }

    public:

        //打印 NFA 的状态信息，包括状态数量、转换数量和每个状态的度数
        void print_states() const
        {
            //输出状态数
            cout << "number of states is " << all_states_.size() << endl;
            //跟踪每个字符的转换数量以及状态的度数
            map<char, int> transitions;
            map<int, int> degrees;

            //初始化
            for(const char& c : alphabet_) transitions.insert(std::make_pair(c, 0));
            transitions.insert(std::make_pair(epsilon_, 0));


            //遍历所有状态，计算每个字符的转换数量和每个状态的度数马，并存放在相应的map中
            for(const auto& s : all_states_)
            {
                for(const char& c : alphabet_)
                {
                    int add = std::distance(s->get_transition(c).first, s->get_transition(c).second);
                    transitions.at(c) += add;
                }
                int degree = s->get_all_transition().size();
                if(degrees.find(degree) != degrees.end())
                {
                    degrees.at(degree) += 1;
                }
                else
                {
                    degrees.insert(std::make_pair(degree, 1));
                }

                transitions.at(epsilon_) += std::distance(s->get_transition(epsilon_).first, s->get_transition(epsilon_).second);

            }

            cout << "[" << epsilon_ << "]"<< "number of transitions are" << transitions.at(epsilon_) << endl;
            for(const char& c : alphabet_)
            {
                cout << "[" << c << "] number of transitions" << transitions.at(c) << endl;
            }
            for(auto & degree : degrees)
            {
                cout << "degree[" << degree.first << "]=" << degree.second << endl;
            }

        }

        bool accepts(const string& str) const
        {
            State* current_state = starting_state_;
            auto e_closure = epsilon_closure(current_state);
            for(const char& c : str)
            {
                set<State*> next_states;
                for(auto s : e_closure)
                {
                    auto add = s->get_transition(c);
                    for(auto iter = add.first; iter != add.second;iter++)
                    {
                        next_states.insert(iter->second);
                    }
                }
                if(next_states.empty()) return false;
                e_closure.clear();
                for(auto s : next_states)
                {
                    auto add = epsilon_closure(s);
                    e_closure.insert(add.begin(), add.end());
                }
            }
            for(auto s : e_closure)
            {
                if(s->is_accepting())
                {
                    return true;
                }
            }
            return false;
        }

        void add_state(State *state, bool starting, bool accepting)
        {
            all_states_.push_back(state);
            if(starting)
            {
                starting_state_ = state;
                state->set_starting(true);
            }
            else
            {
                state->set_starting(false);
            }
            if(accepting)
            {
                accepting_states_.push_back(state);
                state->set_accepting(true);
            }
            else
            {
                state->set_accepting(false);
            }
        }


        NFA symbol_NFA(const char &symbol)
        {
            NFA return_NFA;
            return_NFA.set_epsilon(symbol);
            State* begin = new State;
            State* end = new State;
            begin->add_transition(symbol, end);
            return_NFA.add_state(begin, true, false);
            return_NFA.add_state(end, false, true);
            return return_NFA;
        }

        void add_arc(State *from_state, State *to_state, const char &symbol)
        {
            from_state->add_transition(symbol, to_state);
        }

        [[nodiscard]] const set<char>& get_alphabet() const
        {
            return alphabet_;
        }

        [[nodiscard]] const vector<State*>& get_accepting_states() const
        {
            return accepting_states_;
        }

        [[nodiscard]] State* get_starting_state() const
        {
            return starting_state_;
        }

        char get_epsilon(const char& eps) const
        {
            return epsilon_;
        }

        [[nodiscard]] const vector<State*>& get_all_states() const
        {
            return all_states_;
        }

        set<State*> epsilon_closure(State* s) const
        {
            set<State*> already_evaluated;
            return epsilon_closure_aux(s, already_evaluated);
        }


        void set_starting_state(State* s_state)
        {
            starting_state_ = s_state;
        }

        void set_epsilon(const char& eps)
        {
            epsilon_ = eps;
        }

        void set_alphabet(const set<char>& alphabet)
        {
            alphabet_ = alphabet;
        }

    };


    //正则表达式
    class RE
    {
    public:

        RE()
        :left_(nullptr), right_(nullptr), operation_('\0'), symbol_('\0'), kleene_star_(false)
        {
        }

        RE(const string& regex, const char& epsilon)
        {
            epsilon_ = epsilon;
            recursive_split(regex);
            parse_alphabet(regex);
        }

    private:
        RE* left_;
        RE* right_;
        char operation_;
        char symbol_;
        set<char> alphabet_;
        //克林闭包
        bool kleene_star_;
        //ε
        char epsilon_{};

    private:

        //分治法：递归地将正则表达式分解为更小的子表达式，并分解为左子树和右子树(原理：正砸表达式的递归定义)
        void recursive_split(string regex)
        {
            add_left_and_right();
            left_->epsilon_ = epsilon_;
            right_->epsilon_ = epsilon_;

            //去掉最外层的括号
            if(std::count(regex.begin(), regex.end(), '(') == 1 && regex[0] == '(' && regex.back() == ')')
            {
                regex = regex.substr(1, regex.size() - 2);
            }

            //处理单字符
            if(regex.empty()) return;
            if(regex.size() == 1)
            {
                symbol_ = regex[0];
                return;
            }
            else if(regex[0] == '(') //非单字符，递归分解
            {
                unsigned int brackets_count = 1;
                for(unsigned int ind = 1;ind < regex.size(); ind++)
                {
                    if(regex[ind]== '(')
                        brackets_count++;
                    else if(regex[ind] == ')')
                        brackets_count--;
                    if(brackets_count == 0)
                    {
                        left_->recursive_split(regex.substr(1, ind - 1));
                        if(ind + 1 == regex.size()) //遍历完括号内容后，已经没有更多的字符，处理RE中只有一个括号组的情况
                        {
                            //没有更多的操作符和子表达式
                            operation_= '.';//默认操作是连接
                            right_ = nullptr;
                            return;
                        }
                        else if(regex[ind + 1] == '*')  // 闭包
                        {
                            left_->kleene_star_ = true;
                            if(regex[ind + 2] == '+')   // 或（选择）
                            {
                                operation_ = '+';
                                right_->recursive_split(regex.substr(ind + 3));
                            }
                            else    // 连接
                            {
                                operation_ = '.';
                                right_->recursive_split(regex.substr(ind + 2));
                            }
                        }
                        else if(regex[ind + 1] == '+')
                        {
                            operation_ = '+';
                            right_->recursive_split(regex.substr(ind + 2));
                        }
                        else
                        {
                            operation_ = '.';
                            right_->recursive_split(regex.substr(ind + 1));
                        }
                        break;
                    }
                }
            }
            else    //处理这两个字符的情况
            {
                if(regex.size() == 2)
                {
                    //如:ab, a*
                    if(regex[1] == '*')
                    {
                        symbol_ = regex[0];
                        kleene_star_ = true;
                        return;
                    }
                    else
                    {
                        left_->symbol_ = regex[0];
                        right_->symbol_ = regex[1];
                        operation_ = '.';
                    }
                }
                else if(regex[1] == '+')    //第一个字符是 symbol
                {
                    left_->symbol_ = regex[0];
                    right_->recursive_split(regex.substr(2));
                    operation_ = '+';
                }
                else if(regex[1] == '*')
                {
                    left_->symbol_ = regex[0];
                    kleene_star_ = true;
                    if(regex[2] == '+')
                    {
                        operation_ = '+';
                        right_->recursive_split(regex.substr(3));
                    }
                    else
                    {
                        operation_ = '.';
                        right_->recursive_split(regex.substr(2));
                    }
                    return;
                }
                else
                {
                    if(regex.find('+') != string::npos)
                    {
                        operation_ = '+';
                        left_->recursive_split(regex.substr(0, regex.find('+')));
                        right_->recursive_split(regex.substr(regex.find('+') + 1));
                    }
                    else
                    {
                        left_->symbol_ = regex[0];
                        right_->recursive_split(regex.substr(1));
                        operation_ = '.';
                    }
                }
            }

        }

        void add_left_and_right()
        {
            left_ = new RE;
            right_ = new RE;
        }

        [[nodiscard]] bool is_char() const
        {
            if(symbol_ != '\0' && operation_ != '\0')
                return true;
            return false;
        }

    public:

        NFA convert_to_NFA()
        {
            NFA nfa;
            nfa.set_alphabet(alphabet_);
            nfa.set_epsilon(epsilon_);
            if(is_char())   //如果是单字符，直接返回一个对应于单字符的NFA
            {
                nfa = nfa.symbol_NFA(symbol_);
            }
            else if(operation_ == '+')
            {
                NFA up = left_->convert_to_NFA();
                NFA down = right_->convert_to_NFA();
                State* begin = new State;
                State* end = new State;
                nfa.add_state(begin, true, false);
                nfa.add_state(end, false, true);
                nfa.add_arc(begin, up.get_starting_state(), epsilon_);
                nfa.add_arc(begin, down.get_starting_state(), epsilon_);
                for(auto s : up.get_accepting_states())
                {
                    s->set_accepting(false);
                    nfa.add_state(s, end, epsilon_);
                }
                for(auto s : down.get_accepting_states())
                {
                    s->set_accepting(false);
                    nfa.add_state(s, end, epsilon_);
                }
                for(auto s : up.get_all_states()) nfa.add_state(s, false, false);
                for(auto s : down.get_all_states()) nfa.add_state(s, false, false);
            }
            else if(operation_ == '.')
            {
                NFA first = left_->convert_to_NFA();
                NFA second = right_->convert_to_NFA();
                nfa.set_starting_state(first.get_starting_state());
                for(auto s : first.get_accepting_states())
                {
                    s->set_accepting(false);
                    nfa.add_arc(s, second.get_starting_state(), epsilon_);
                }
                for(auto s : first.get_all_states())
                {
                    if(s->is_starting() && !s->is_accepting())
                        nfa.add_state(s, true, false);
                    else if(s->is_starting() && s->is_accepting())
                        nfa.add_state(s, true, true);
                    else
                        nfa.add_state(s, false, false);
                }
                for(auto s : second.get_all_states())
                {
                    if(s->is_accepting())
                        nfa.add_state(s, false, true);
                    else
                        nfa.add_state(s, false, false);
                }
                nfa.get_starting_state()->set_starting(true);
            }
            if(kleene_star_)
            {
                NFA return_NFA = *(new NFA);
                return_NFA.set_epsilon(epsilon_);
                auto begin = new State;
                auto end = new State;
                return_NFA.add_state(begin, true, false);
                return_NFA.add_state(end, false, true);
                return_NFA.add_arc(begin, nfa.get_starting_state(), epsilon_);
                for(auto s : nfa.get_accepting_states())
                {
                    s->set_accepting(false);
                    return_NFA.add_arc(s, end, epsilon_);
                    return_NFA.add_arc(s, nfa.get_starting_state(), epsilon_);
                }
                return_NFA.add_arc(begin, end, epsilon_);
                for(auto s : nfa.get_all_states())
                    return_NFA.add_state(s, false, false);
                return return_NFA;
            }
            else
            {
                return nfa;
            }
        }

        void parse_alphabet(const string& regex)
        {
            vector<char> ignored_chars = {epsilon_, '(', ')', '*', '+'};
            for(const char& c : regex)
            {
                if(std::find(ignored_chars.begin(), ignored_chars.end(), c) == ignored_chars.end())
                {
                    alphabet_.insert(c);
                }
            }
        }

    };


    //Table
    class Table
    {
    public:
        Table(const vector<State*>& all_states, set<string> alphabet)
        {
            //记录状态集合中的所有状态的 name_
            for(State* s : all_states) states_names_.insert(s->get_name());

            for(int i = 1;i < states_names_.size();i++)
            {
                vector<bool> to_push;
                for(int k = 0;k < i;i++)
                {
                    to_push.push_back(false);
                }
                cells_.push_back(to_push);
            }
            states_ = all_states;
            alphabet_ = alphabet;
        }

        Table() = default;

    private:
        //存储FA的字母表
        set<string> alphabet_;
        //表示状态之间的连接关系（状态转移表）表示从状态i在输入j上是否有转移
        vector<vector<bool>> cells_;
        //存储所有状态名称的集合
        set<string> states_names_;
        //状态集合
        vector<State*> states_;

    public:
        void set_bool(const string& A, const string& B, bool b)
        {
            //定位 A 和 B 在 states_names_ 中的位置
            auto iter = std::find(states_names_.begin(), states_names_.end(), A);
            auto pos_A = std::distance(states_names_.begin(), iter);
            iter = std::find(states_names_.begin(), states_names_.end(), B);
            auto pos_B = std::distance(states_names_.begin(), iter);
            if(pos_B < pos_A)
                std::swap(pos_A, pos_B);
            cells_[pos_B - 1][pos_A] = b; //从pos_B状态到状态pos_A是（否）存在转移取决于 b
        }

        void print() const
        {
            int i = 0;
            for(const auto& name : states_names_) {
                if (name == *states_names_.begin())
                    continue;
                cout << name;
                for (int k = 0; k < cells_[i].size(); k++)
                {
                    cout << "  ";
                    if(cells_[i][k])  //如果从i到k存在转移，则
                        cout << "X";
                    else    //不存在，则
                        cout << "-";
                }
                i++;
                cout << endl;
            }
        }

        //获取 两个状态之间的等价性（bool），与set_bool 相似
        [[nodiscard]] bool get_bool(const string& A, const string& B) const
        {
            //如果相等。直接返回
            if(A == B) return true;
            auto iter_A = std::find(states_names_.begin(), states_names_.end(), A);
            auto pos_A = std::distance(states_names_.begin(), iter_A);
            auto iter_B = std::find(states_names_.begin(), states_names_.end(), B);
            auto pos_B = std::distance(states_names_.begin(), iter_B);

            if(pos_B < pos_A)
                std::swap(pos_A, pos_B);
            return cells_[pos_B - 1][pos_A];
        }

        void arrange_table()
        {
            for(const auto& sa: states_)
            {
                for(const auto& sb : states_)
                {
                    if(sa->is_accepting() && !sb->is_accepting())
                    {
                        set_bool(sa->get_name(), sb->get_name(), true); //设置转移
                    }
                }
            }
            bool final = false;
            while(!final)
            {
                final = true;
                for(auto sa: states_)
                {
                    for(auto sb : states_)
                    {
                        if(sa == sb)
                            continue;
                        if(get_bool(sa->get_name(), sb->get_name()))
                        {
                            for(auto sym : alphabet_)   //如果存在转移，寻找这样的转移组合
                            {
                                for(auto sc : states_)
                                {
                                    for(auto sd : states_)
                                    {
                                        if(sc == sd)
                                            continue;
                                        State* trans_c = sc->get_transition(sym[0]).first->second;
                                        State* trans_d = sd->get_transition(sym[0]).first->second;
                                        if((trans_c == sa && trans_d == sb) || (trans_c == sb && trans_d == sa))
                                        {
                                            if(get_bool(sc->get_name(), sd->get_name()))
                                                continue;
                                            set_bool(sc->get_name(), sd->get_name(), true);
                                            final = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        //获取合并后的状态集合。通过查找等价状态，创建新的合并状态，并设置这些状态的转移关系
        [[nodiscard]] vector<State*> get_merged_states() const
        {
            vector<State*> return_states;
            for(State* s : states_) //跳过多字符 names， 获取 equivalent_states
            {
                //获取状态集合中各状态的等价状态
                vector<State*> equivalent_states = get_equivalent_states(s);
                //state_name，初始化为 {
                string state_name = "{";
                set<string> names;
                names.insert(s->get_name());
                for(auto eq : equivalent_states)
                {
                    names.insert(eq->get_name());
                }
                //加入所有的name
                for(const auto& name : names)
                {
                    state_name.append(name);
                    if(name != *names.rbegin())
                        state_name.append(", "); // ", "作为分割符
                }
                state_name.append("}");//结尾
                bool is_starting = false;
                bool is_accepting = false;
                for(auto state : equivalent_states)
                {
                    if(state->is_accepting()) is_accepting = true;
                    if(state->is_starting()) is_starting = true;
                }
                if(s->is_accepting()) is_accepting = true;
                if(s->is_starting()) is_starting = true;
                auto to_add = new State(state_name, is_starting, is_accepting);
                return_states.push_back(to_add);
            }
            //移除重复的状态，初始化一个 temp
            vector<State*> temp;
            for(State* s : return_states)
            {
                int duplication = 0;
                for(State* added : temp)
                {
                    if(added->get_name() == s->get_name())
                        duplication++;
                }
                if(temp.empty() || duplication == 0)
                    temp.push_back(s);  //将没有重复的状态添加到 temp 中
            }
            //更新 return_states
            return_states = temp;
            for(State* s : return_states)   //设置状态间的转移
            {
                for(auto sym : alphabet_)
                {
                    string name_of_referenced_state =
                            s->get_name().substr(1, s->get_name().find(',') - 1);
                    if(name_of_referenced_state.back() == '}')  //如果第一个状态的最后一个字符是'}',则去掉'}'
                        name_of_referenced_state =
                                name_of_referenced_state.substr(0, name_of_referenced_state.size() - 1);
                    State* from_reference;
                    for(auto ref : states_)     //在state_ 中查找到第一个状态的等价状态
                    {
                        if(ref->get_name() == name_of_referenced_state)
                        {
                            from_reference = ref;
                            break;
                        }
                    }
                    State* to_reference = from_reference->get_transition(sym[0]).first->second;
                    State* to_state;
                    for(auto possible_destination : return_states)  //查找包含 to_reference name的状态并赋值给to_state
                    {
                        if(possible_destination->get_name().find(to_reference->get_name()) !=
                        possible_destination->get_name().npos)
                            to_state = possible_destination;
                    }
                    s->add_transition(sym[0], to_state);
                }
            }
            return return_states;
        }

        vector<State*> get_equivalent_states(State* s) const
        {
            vector<State*> return_states;
            for(const auto& ss : states_)
            {
                if(!get_bool(s->get_name(), ss->get_name()))    //不存在转移
                    return_states.push_back(ss);
            }
            return return_states;
        }
    };


    //DFA（属于NFA的特殊种类）
    class DFA
    {
    public:

        DFA()
        :starting_state_(new State)
        {
        }

    private:
        //字母表
        set<string> alphabet_;
        //接收状态集合
        vector<State*> accepting_states_;
        //起始状态
        State* starting_state_;
        //所有状态的集合
        vector<State*> all_states_;
        //符号表
        Table table_;
    public:
        bool operator==(const DFA& rhs_dfa) const
        {
            vector<State*> vector_union = all_states_;
            vector_union.insert(vector_union.end(), rhs_dfa.all_states_.begin(), rhs_dfa.all_states_.end());
            Table table_union = Table(vector_union, alphabet_);
            table_union.arrange_table();
            table_union.print();
            return !table_union.get_bool(starting_state_->get_name(), rhs_dfa.starting_state_->get_name());
        }

        bool accepts(const string& inp) const
        {
            State* current = starting_state_;
            for(const auto& i : inp)
            {
                current = current->get_transition(i).first->second;
            }
            return current->is_accepting();
        }

        DFA minimized()
        {
            DFA return_DFA;
            table_ = Table(all_states_, alphabet_);
            table_.arrange_table();
            return_DFA.alphabet_ = alphabet_;
            return_DFA.all_states_ = table_.get_merged_states();
            for(const auto& s : return_DFA.all_states_)
            {
                if(s->is_starting())
                    return_DFA.starting_state_ = s;
                if(s->is_accepting())
                    return_DFA.accepting_states_.push_back(s);
            }
            return return_DFA;
        }

        void print_table() const
        {
            table_.print();
        }
    };









}



#endif //MY_COMPILER_STATE_TRANSITION_H
