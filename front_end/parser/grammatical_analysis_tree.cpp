//
// Created by 86198 on 2024/11/15.
//

#include "grammatical_analysis_tree.h"


void my_compiler_tree::GrammaticalAnalysisTree::genDOT(const string &filename) const
{
    fstream out;
    out.open(filename);
    out << "digraph G {\n";
    out << "graph[color=\"#242038\"]\n";
    out << "node[color=\"#242038\", shape=square]\n";
    out << "edge[color=\"#242038\"]\n";
    int rootNum = 0; // 节点计数器
    genDOT_aux(root_, rootNum, out);
    out << "}\n";
}



void my_compiler_tree::GrammaticalAnalysisTree::genDOT_aux(
        const std::shared_ptr<my_compiler_tree::GrammaticalAnalysisTreeNode> &node, int &nodeCount, fstream &out,
        bool isLeft) const
{
    if (!node) return; // 如果节点为空，返回

    // DOT for current node
    string nodeLabel = "n" + std::to_string(nodeCount) + " [label=< <B>" + std::to_string(node->op_) + "</B>";
    if (!node->value_.empty()) {
        if(node->value_.size() > 2 && node->value_[0] == '<' && node->value_[node->value_.size() - 1] == '>')
        {
            size_t index = node->value_.find('>');
            string value = node->value_.substr(1, index - 1);
            nodeLabel += "<BR/>&lt;" + value + "&gt;";
        }
        else if(node->value_[node->value_.size() - 1] == '\'')
        {
            size_t index = node->value_.find('>');
            string value = node->value_.substr(1, index - 1);
            nodeLabel += "<BR/>&lt;" + value + "&gt;'";
        }
        else
        {
            string value = node->value_;
            if(node->value_ == "empty")
            {
                value = "epsilon";
            }
            nodeLabel += "<BR/>" + value;
        }
    }
    nodeLabel += ">";
    nodeLabel += (isLeft ? R"(, color="#FF595E", fontcolor="#FF595E")"
                         : R"(, color="#274690", fontcolor="#274690")");
    if (node->is_leaf_) {
        nodeLabel += ", shape=egg";
    }
    nodeLabel += "]\n";

    out << nodeLabel;

    int currentNodeCount = nodeCount;
    nodeCount++; // 计数器自增
    for (const auto &child: node->child_) {
        out << "n" + std::to_string(currentNodeCount) + "->n" + std::to_string(nodeCount) + "\n";
        genDOT_aux(child, nodeCount, out); // 递归生成DOT格式的子节点
    }
}


std::shared_ptr<my_compiler_tree::GrammaticalAnalysisTreeNode>
my_compiler_tree::GrammaticalAnalysisTreeNode::get_node(const string &value)
{
    if(child_.empty())
    {
        exit(0);
    }
    else if(child_.size() == 1)
    {
        return child_[0];
    }
    else
    {
        for(const auto& c : child_)
        {
            if(c->value_ == value)
            {
                return c;
            }
        }
    }
    return nullptr;
//    static shared_ptr<GrammaticalAnalysisTreeNode> nullPtr = nullptr;
//    return nullPtr; // 如果没有找到，返回 nullptr 引用
}
