//
// 创建者：86198；日期：2024/11/15。
//

#ifndef MY_COMPILER_GRAMMATICAL_ANALYSIS_TREE_H
#define MY_COMPILER_GRAMMATICAL_ANALYSIS_TREE_H


#include"../../header_files.h"

namespace my_compiler_tree {

    using std::shared_ptr;

    //《编译原理》语法分析树结点
    /*
     * 我们将使用具有适当数量的字段的对象来实现一棵语法分析树的各个结点。
     * 每个对象将有一个 op 字段，也就是这个结点的标号。这些对象将具有如下所述的其他字段：
     *      * 如果结点是一个 叶子，那么对象将有一个附加的域来存放这个叶子结点的词法值。
     *      构造函数 Leaf(op, val) 创建一个叶子对象。我们也可以把结点看作记录，那么Leaf
     *      就会返回一个指向与叶子节点对应的新纪录的指针。
     *      * 如果结点是内部结点，那么它的附加字段的个数和该结点在语法树中的子节点个数相同。
     *      构造函数 Node 带有两个或多个参数：Node(op, c1, c2, ..., ck)，该函数创建一个
     *      对象，第一个字段的值为 op, 其余 k 个字段的值为 c1, ... , ck
     * */




    struct GrammaticalAnalysisTreeNode : public std::enable_shared_from_this<GrammaticalAnalysisTreeNode> //兄弟孩子记法
    {
        //结点的标号
        int op_;
        //词法值
        string value_;
        //父节点指针
        struct shared_ptr<GrammaticalAnalysisTreeNode> parent_;
        //兄弟指针
        struct shared_ptr<GrammaticalAnalysisTreeNode> sibling_;
        //子节点
        vector<shared_ptr<GrammaticalAnalysisTreeNode>> child_;
        //是否是叶子节点
        bool is_leaf_;


        GrammaticalAnalysisTreeNode() : op_(0), is_leaf_(false) {}

        GrammaticalAnalysisTreeNode(const int &op, string value) : op_(op), value_(std::move(value)), is_leaf_(false) {}

        GrammaticalAnalysisTreeNode(const int &op, string value, bool leaf) : op_(op), value_(std::move(value)),
                                                                              is_leaf_(leaf) {}

        ~GrammaticalAnalysisTreeNode() = default;

        shared_ptr<GrammaticalAnalysisTreeNode> get_node(const string &value);


        friend std::ostream &operator<<(std::ostream &out, const shared_ptr<GrammaticalAnalysisTreeNode> &node)
        {
            if (node->is_leaf_)
            {
                out << "叶子结点 " << std::setw(2) << node->op_ << ", 值：";
            }
            else
            {
                out << "内部结点 " << std::setw(2) << node->op_ << ", 值：";
            }
            out << std::setiosflags(ios::left) << std::setw(25) << node->value_;
            return out;
        }


    };


    //目标：重写语法分析树各结点的结构


    struct GrammaticalAnalysisTree {
        //推导式
        vector<string> infer_;
        shared_ptr<GrammaticalAnalysisTreeNode> root_;


        // 打印树的结构
        void genDOT(const string& filename) const;


    private:

        void genDOT_aux(const shared_ptr<GrammaticalAnalysisTreeNode> &node, int &nodeCount, fstream& out, bool isLeft = false) const;

    };


} // namespace my_compiler_tree

#endif //MY_COMPILER_GRAMMATICAL_ANALYSIS_TREE_H
