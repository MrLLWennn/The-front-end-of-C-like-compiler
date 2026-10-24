//
// 创建者：86198；日期：2024/11/1。
//

#ifndef MY_COMPILER_CONTEXT_FREE_GRAMMAR_H
#define MY_COMPILER_CONTEXT_FREE_GRAMMAR_H




//该头文件用于 上下文无关文法
/*
 *
 * 上下文无关文法
 *
 * （形式如：stmt -> if(expr) stmt else stmt    ）
 * 1. 定义：由终结符号，非终结符号，一个开始符号和一组产生式组成。
 *      1）终结符号：是组成串的基本符号。“词法单元名字”是“终结”符号的同义词。如：if, else, {, }
 *      2）非终结符：表示串的集合的语法变量。如：stmt, expr。非终结符号表示的串集合用于定义由文法生成的语言。
 *     非终结符给出了语言的层次结构，而这种层次结构是语法分析和翻译的关键
 *      3）开始符号：在一个文法中，指定一个非终结符为开始符号。这个符号表示的串集合就是这个文法生成的语言。
 *     按照惯例，首先列出开始符号的产生式。
 *      4）产生式：描述了将终结符号和非终结符号组成串的方法。由以下元素组成：
 *          （1）一个被称为产生式头部或左部的非终结符，这个产生式定义了这个头所代表的串集合的一部分。
 *          （2）符号 ->。有时也用 ::= 代替箭头
 *          （3）一个由零个或多个终结符号与非终结符号组成的产生式体或右部。产生式体中的成分描述了产生式头上的非终结符号所对应的串的某种构造方式。
 * 2.约定
 *
 *
 * 推导
 *      将产生式看作是重写规则，就可以从推导的角度
 *
 * */


#include"../../header_files.h"
#include <cstddef>
#include<unordered_map>
#include<unordered_set>


namespace CFG
{
    using std::unordered_map;
    using std::unordered_set;


    //上下文无关文法
    class ContextFreeGrammar
    {
    private:
        //输入输出文件对象
        fstream file_in_;
        fstream file_out_;
        //终结符的展示标签；文法符号的收集不依赖这张表
        unordered_map<string, string> terminal_type_;
        //终结符集合
        vector<string> terminal_set_;
        //非终结符集合
        vector<string> non_terminal_set_;
        //存储所有产生式
        unordered_map<string, vector<string>> productions_;
        //FIRST 集合
        unordered_map<string, vector<string>> first_;
        //FOLLOW 集合
        unordered_map<string, vector<string>> follow_;

    public:

        //预测分析表
        struct PredictiveAnalysisTable
        {
            unordered_map<string, unordered_map<string, vector<string>>> table_;
            vector<string> Vn_;
            vector<string> Vt_;
            unordered_map<string, vector<string>> productions_;
        };

        PredictiveAnalysisTable predictive_analysis_table_;

    public:

        ContextFreeGrammar()
        {
            terminal_type_ = {
                    {"<", "比较运算符"},
                    {"<=", "比较运算符"},
                    {">", "比较运算符"},
                    {">=", "比较运算符"},
                    {"!=", "比较运算符"},
                    {"==", "比较运算符"},
                    {"=", "赋值运算符"},
                    {"+", "算术运算符"},
                    {"-", "算术运算符"},
                    {"*", "算术运算符"},
                    {"/", "算术运算符"},
                    {"^", "算术运算符"},
                    {"+=", "复合赋值运算符"},
                    {"-=", "复合赋值运算符"},
                    {"/=", "复合赋值运算符"},
                    {"*=", "复合赋值运算符"},
                    {"^=", "复合赋值运算符"},
                    {"++", "自增减运算符"},
                    {"--", "自增减运算符"},
                    {";", "分隔符"},
                    {",", "分隔符"},
                    {":", "分隔符"},
                    {"(", "分隔符"},
                    {")", "分隔符"},
                    {"[", "分隔符"},
                    {"]", "分隔符"},
                    {"{", "分隔符"},
                    {"}", "分隔符"},
                    {"int", "关键字"},
                    {"void", "关键字"},
                    {"float", "关键字"},
                    {"double", "关键字"},
                    {"bool", "关键字"},
                    {"if", "关键字"},
                    {"else", "关键字"},
                    {"while", "关键字"},
                    {"for", "关键字"},
                    {"break", "关键字"},
                    {"switch", "关键字"},
                    {"case", "关键字"},
                    {"default", "关键字"},
                    {"continue", "关键字"},
                    {"const", "关键字"},
                    {"struct", "关键字"},
                    {"return", "关键字"},
                    {"true", "布尔字面量"},
                    {"false", "布尔字面量"},
                    {"identifier", "标识符"},
                    {"identifier-function", "函数标识符"},
                    {"identifier-struct", "结构体标识符"},
                    {"identifier1", "赋值目标标识符"},
                    {"number", "数值"},
                    {"empty", "ε"}
            };

        }

    public:
        void remove_left_recursion_and_factor()
        {
            //处理直接左递归以及提取公共前缀因子
            remove_direct_left_recursion_and_factor_aux(grammar_file, LL1grammar_file);
            divide_line();
            //处理间接左递归
        }

        void split_grammar()
        {
            split_grammar_aux(LL1grammar_file, processed_grammar_file);
        }

        void cal_Vn_Vt()
        {
            //初始化产生式
            generate_productions(processed_grammar_file);
            cal_Vn_Vt_aux(processed_grammar_file, Vn_Vt_file);
        }

        //计算first, follow集合
        void cal_first_and_follow()
        {
            output_file(file_out_, first_follow_file);
            cal_first_aux();
            cal_follow_aux();
            file_out_.close();
        }

        //以图表格式标准输出
        void generate_predictive_analysis_table()
        {
            input_file(file_in_, processed_grammar_file);
            output_file(file_out_, pred_table_file);
            generate_predictive_analysis_table_aux();
            file_in_.close();
            file_out_.close();
        }

        void context_free_grammar_pretreatment()
        {
            //将原始文法文件 processed_grammar.txt 进行消除直接左递归和提取公共因子
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[4] << "------------************" << endl;
            cout << "读入文件" << grammar_file << endl;
            cout << "文件写入" << LL1grammar_file << endl;
            remove_left_recursion_and_factor();
            divide_line();

            //对其进行划分
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[5] << "------------************" << endl;
            cout << "读入文件" << LL1grammar_file << endl;
            cout << "文件写入" << processed_grammar_file << endl;
            split_grammar();
            divide_line();

            //终结符以及非终结符集合
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[6] << "------------************" << endl;
            cout << "读入文件" << processed_grammar_file << endl;
            cout << "文件写入" << Vn_Vt_file << endl;
            cal_Vn_Vt();
            divide_line();

            //FIRST 和 FOLLOW集合
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[7] << "------------************" << endl;
            cout << "文件写入" << first_follow_file << endl;
            cal_first_and_follow();
            divide_line();

            //构建 预测分析表
            cout << "************------------ 过程：" << period[3] << "----步骤：" << step[8] << "------------************" << endl;
            cout << "读入文件：" << processed_grammar_file << endl;
            cout << "文件写入" << pred_table_file << endl;
            generate_predictive_analysis_table();
            divide_line();

        }


    private:

        void generate_productions(const string& from_file);

        //消除间接左递归
        //void remove_indirect_left_recursion_aux(const string& from_file, const string& to_file);


        //消除直接左递归和公共因子
        /*
         * 1.直接左递归消除：左递归会导致递归下降分析器无限递归。可以通过重新格式化来消除左递归
         * 2.公共因子：当两个或多个产生式共享相同的前缀时，称为公共左因子。提取公共左因子可以避免分析器的歧义性
         * */
        void remove_direct_left_recursion_and_factor_aux(const string& from_file, const string& to_file);

        void split_grammar_aux(const string& from_file, const string& to_file);

        //计算终结符 Vt 和 非终结符Vn集合
        void cal_Vn_Vt_aux(const string& from_file, const string& to_file);

        bool is_non_terminal(const string &symbol)
        {
            return std::find(non_terminal_set_.begin(), non_terminal_set_.end(), symbol) != non_terminal_set_.end();
        }

        bool is_terminal(const string &symbol)
        {
            return std::find(terminal_set_.begin(), terminal_set_.end(), symbol) != terminal_set_.end();
        }

        //非终结符计算的递归过程
        void _cal_first_aux(const string& symbol);

        void cal_first_aux();

        // 求 symbols[begin..end) 的 FIRST；terminals 不存 empty，可空性单独记录。
        struct SequenceFirst
        {
            std::vector<std::string> terminals;
            bool nullable = true;
        };

        SequenceFirst first_of_sequence(
            const std::vector<std::string>& symbols, std::size_t begin) const;

        void cal_follow_aux();


        //生成预测分析表
        void generate_predictive_analysis_table_aux();


    };// class context_free_grammar;


}// namespace CFG




#endif //MY_COMPILER_CONTEXT_FREE_GRAMMAR_H
