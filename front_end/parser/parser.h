//
// 创建者：86198；日期：2024/10/20。
//

#ifndef MY_COMPILER_PARSER_H
#define MY_COMPILER_PARSER_H

#include "my_compiler/compilation_error.hpp"



/*
 * 语法分析器
 * 1. 语法分析器从词法分析器中获得一个有词法单元组成的串，并验证这个串可以由源语言的文法生成。
 * 2. 我们期望语法分析器能够以易于理解的方式报告语法错误，并且能够从常见的错误中恢复并处理其余部分。
 * 3. 对于良构的程序，语法分析器会构造出一棵语法分析树，对源程序的就检查和翻译动作可以和语法分析交错完成。
 * 4. 处理文法的语法分析器大体上可以分为三种：自底向上，自顶向下和通用（效率较低，不适用于编译器）
 *      1）自顶向下的方法从语法分析器的顶部（根节点）开始向底部（叶子节点）构造一棵 语法分析树
 *      2）自底向上的方法则是从叶子节点开始，逐渐向根节点方向构造。
 *      3）语法分析器的输入总是按照从左到右的方式被扫描，每次扫描一个符号
 * 5. 在实践中，语法分析过程可能会包括多个任务，，比如将词法单元的信息收集到符号表中，进行类型检查和其他类型的语义分析，以及生成中间代码
 * 6. 语法错误的处理（***）
 *  ————程序中常见的 集中错误：
 *      1）词法错误：包括标识符，关键字或运算符的拼写错误和没有在字符串文本上正确加上引号
 *      2）语法错误：包括分号放错位置，花括号，即"{"或"}"多余或缺少。
 *      另一个例子是 一个 case 语句的外围没有相应的switch语句，但是语法分析器通常允许这种情况的出现，当编译器要生成代码时才发现这个错误
 *      3）语义错误：包括运算符和运算分量之间类型的不匹配；例如 void 函数却 return 了值
 *      4）逻辑错误：可以因程序员的错误推断而引发的任何错误。比如： == 和 = 的错误使用。程序是良构的，但是没有正确反映程序员的意图。
 *    LL和LR方法嫰够在第一时间发现错误。即 ，来自词法分析器的词法单元流不能根据该语言的文法进一步分析就会发现错误。
 *    更精确地讲，它们具有可行前缀特性，一旦发现输入的某个前缀不能通过添加一些符号而形成这个语言的串，就可以立刻检查到语法错误。
 *
 *    语法分析器错误处理程序目标：
 *      1）清晰精确地报告出现的错误
 *      2）能很快地从各个错误中恢复，以继续检查后面的错误
 *      3）尽可能少地增加处理正确程序的开销
 *
 *
 *  ————错误恢复策略
 *      1）恐慌模式的恢复
 *          语法分析器一旦发现错误就不断丢弃输入中的符号，一次丢弃一个符号，直到找到同步词法单元（通常是界限符，比如分号或者,在源程序中的作用是无二义性，清晰的}）集合中的某个元素为止。
 *      恐慌模式的错误纠正方法常常会跳过大量输入，不检查被跳过部分的其他错误。但是较为简单，且保证不会进入无限循环。
 *      2）短语层次的恢复
 *          当发现一个错误时，语法分析器可以在余下的输入上进行局部性纠正。它可能将余下输入的某个前缀替换为另一个串，使语法分析器得以继续分析。
 *      常用的局部纠正方法包括将一个逗号替换为分号，删除一个多余的分号或者插入一个遗漏的分号。当然，必须小心选择替换方法，以避免进入无限循环。
 *      主要不足在于，它难以处理实际错误发生在被检测位置之前的情况。
 *      3）错误产生式
 *          通过预测可能遇到的常见错误，可以在当前语言的文法中加入特殊的产生式。这些产生式能够产生含有错误的构造，从而基于增加错误产生式的文
 *      法构造得到一个语法分析器。如果语法分析过程中使用了某个错误产生式，语法分析器就检测到了一个预期的错误。语法分析器能够据此生成适当的错
 *      误诊断信息，指出在输入中识别出的错误构造。
 *      4）全局纠正
 *          有些算法可以选择一个最小的改动序列，得到开销最低的全局性纠正方法。给定一个不正确的输入 x 和文法 G，这些算法将找出一个相关串 y 的
 *      语法分析树，使得将 x 转换为 y 所需要的插入、删除和改变的词法单元数量最少。但是，从时间和空间的角度，开销过大。
 * */

#include"../lexer/lexer.h"
#include"context_free_grammar.h"
#include"grammatical_analysis_tree.h"
#include<stack>



/*
 * 1.自顶向下的语法分析
 *      可以被看作是为了输入串构造语法分析树的问题，它从语法分析树的根节点开始，按照先根次序（深度优先地）创建这棵语法分析树的各个结点。
 *   自顶向下语法分析可以被看作是寻找输入串的最左推导的过程。
 *   --- 递归下降的语法分析
 *   1）一个递归下降语法分析程序由一组过程组成，每个非终结符号有一个对应的过程。
 *   2）程序的执行从开始符号对应的过程开始，如果这个过程的过程体扫描了整个输入串，它就停止执行并宣布语法分析成功完成。
 *   3）通用的递归下降分析技术可能需要回溯。即，它可能需要重复扫描输入。然而，在对程序设计语言的构造进行语法分析时很少需要回溯，
 *   因此需要回溯的语法分析器并不常见。人们更加倾向于基于表格的方法。
 *
 * 2. 对于称为LL（1）的文法，可以构造预测分析表，即不需要回溯的递归下降语法分析器。
 * LL(1) 中的第一个“L”表示从左至右扫描输入，第二个“L“表示产生最左推导，而”1“的则表示在每一步中只需要向前看一个输入符号来决定语法析动作。
 * 3. 构造预测分析器的转换图
 *      转换图有助于预测分析器的可视化。要构造一个文法的转换图，首先要消除左递归，然后对文法提取左公因子，然后对每个非终结符号A：
 *          1）创建一个初始状态和一个初始状态
 *          2）对于每一个产生式 A->X1X2...Xn，创建一个从初始状态到结束状态的路径，路径中各条边的标号为 X1, X2,..., Xn。
 *          如果A -> ε，那么这条路径就是一条标号为ε的边。
 * 4. FIRST 和 FOLLOW集合
 *      在自顶向下语法分析器过程中，有助于我们根据下一个输入符号来选择应用哪个产生式。
 *      在恐慌模式的错误恢复中，由 FOLLOW 产生的词法单元集合可以作为同步词法单元。
 *
 *      有关 FIRST 和 FOLLOW集合的描述详见我的博文 —— https://blog.csdn.net/m0_74985290/article/details/141905497?spm=1001.2014.3001.5501
 * */



namespace my_compiler
{

    using namespace CFG;
    using namespace my_compiler_token;
    using namespace my_compiler_tree;
    using std::stack;

    class Lexer;


    //此语法分析器出于处理较为简易的程序，使用LL语法分析技术
    /*
     * 非递归的预测分析
     *
     * 1. 它显式地维护一个栈结构，而不是通过递归调用的方式隐式维护一个栈。这样的语法分析器的可以模拟最左推导的过程。
     * 2. 由分析表驱动的语法分析器有一个输入缓冲区，一个包含了文法符号序列的栈，一个 预测分析表，以及一个输出流。
     * 3. 它的输入缓冲区中包含要进行语法分析的串，串后面要跟着结束标记 $ 。
     * 4. 我们复用符号 $ 来标记栈底。在开始时刻，栈中 $ 的上方是开始符号 S。
     * 5. 语法分析器由一个程序控制。考虑栈顶符号 X 和当前输入符号 a。如果 X 是一个非终结符号，该分析器查询预测分析表中
     * 的条目 M[X, a] 来选择一个 X 产生式。（这里可以执行以此为附加的代码，比如构造一个语法分析树结点的代码）否则，它
     * 检查终结符号 X 和 当前输入符号 a 是否匹配
     * */
    class Parser
    {
        friend class Lexer;

        using Node = GrammaticalAnalysisTreeNode;

    public:
        Parser()
        {
            //栈初始：用 $ 来标记栈底
            stack_.emplace("$");
            //开始符号 <S> 作为栈顶
            stack_.emplace("<S>");
        }

    public:

        void grammatical_analysis(const vector<Token>& tokens, bool emit_visual_tree = true)
        {
            //先对文法进行消除左递归，提取左公因子，计算 Vn，Vt终结符集合以及 FIRST 和 FOLLOW集合, 预测分析表
            grammar_.context_free_grammar_pretreatment();
            tokens_ = tokens;
            predictive_analysis_table_ = grammar_.predictive_analysis_table_;

            //预测分析
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[9] << "------------************" << endl;
            cout << "文件写入" << stack_file << endl;
            predictive_analysis();
            divide_line();

            if (emit_visual_tree)
            {
                //生成语法树
                cout << "************------------ 过程：" << period[3] << "----步骤：" << step[10] << "------------************" << endl;
                cout << "文件写入" << tree_file << endl;
                generate_grammatical_analysis_tree();
                divide_line();
            }
        }


        //生成语法分析树
        void generate_grammatical_analysis_tree()
        {
            output_file(file_out_, tree_file);
            generate_grammatical_analysis_tree_aux();
            tree_.genDOT(tree_dot_file);
            file_out_.close();
        }


        void predictive_analysis()
        {
            output_file(file_out_, stack_file);
            predictive_analysis_aux();
            file_out_.close();
        }


        //生成符号表

        //错误处理
        void error()
        {
            error_aux();
        }

    private:

        bool is_non_terminal(const string& value);

        bool is_terminal(const string& value);

        void error_aux();

        void generate_grammatical_analysis_tree_aux();

        void predictive_analysis_aux();

    private:
        //文件操作对象
        fstream file_in_;
        fstream file_out_;
        //CFG 上下文无关文法
        ContextFreeGrammar grammar_;


        //显式维护的栈结构
        template<class T>
        struct Stack
        {
            vector<T> value_;
            size_t size_;

            Stack() : size_(0){}

            void emplace(const T& value)
            {
                value_.emplace_back(value);
                size_++;
            }

            [[nodiscard]] T top() const
            {
                if (size_ == 0)
                    throw std::logic_error("parser stack underflow");
                return value_[size_ - 1];
            }

            void pop()
            {
                if(size_ == 0)
                {
                    throw std::logic_error("parser stack underflow");
                }
                else
                {
                    value_.pop_back();
                    size_--;
                }
            }

            [[nodiscard]] size_t size() const noexcept
            {
                return size_;
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return this->size_ == 0;
            }

            friend std::ostream& operator<<(std::ostream& out, const Stack& rhs)
            {
                out << "当前栈中的元素个数为：" << std::setiosflags(ios::right) << std::setw(2) << rhs.size_ << "     栈顶| ";
                for(int i = rhs.size_ - 1;i >= 0;i--)
                {
                    out << rhs.value_[i] << " ";
                }
                return out;
            }
        };

        //预测分析栈
        Stack<string> stack_;
        //词法分析器的获得的 词法单元序列
        vector<Token> tokens_;
        //预测分析表
        CFG::ContextFreeGrammar::PredictiveAnalysisTable predictive_analysis_table_;
        //语法分析树
        GrammaticalAnalysisTree tree_;
    };
}


#endif //MY_COMPILER_PARSER_H
