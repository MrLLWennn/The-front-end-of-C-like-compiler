//
// Created by 86198 on 2024/10/19.
//

#ifndef FUNDAMENTALS_OF_COMPILING_HEADER_FILES_H
#define FUNDAMENTALS_OF_COMPILING_HEADER_FILES_H



//此头文件包含所有用到的库，常量声明，文件相关操作

//约定：
/*
 * 为简化起见， 此编译器省略：
 * 1. 静态作用域
 * 2. class 声明
 * 3. 逻辑运算符，位运算符，条件运算符，域运算符
 * 4. 函数内局部变量必须放在函数体开头的位置（由于文法设计原因，作为 bug ，待解决）
 * */


#pragma once


#include<iostream>
#include<string>
#include<cstring>
#include<vector>
#include<map>
#include<set>
#include<algorithm>
#include<cctype>
#include<cstdio>
#include<fstream>
#include<memory>
#include<windows.h>
#include<iomanip>
#include<tr1/tuple>
#include<set>
#include<list>
#include<sys/stat.h>
#include<queue>





using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::string;
using std::fstream;
using std::ios;
using std::list;
using std::set;
using std::cerr;
using std::queue;
using std::shared_ptr;




//------------------------------------------------------------------------------


//常量定义


//此编译器编译过程的数量上限
const int period_num = 10;
//此编译器个过程的编译步骤的数量上限
const int step_num = 20;


// 源程序
static const string source_file             = "../files/00-source.txt";
// 去注释后的文件
static const string uncomment_file          = "../files/01-uncomment.txt";
// 预处理完成的文件
static const string pretreatment_file       = "../files/02-pretreatment.txt";
// 词法分析结果存储文件
static const string lex_file                = "../files/03-lexer.txt";
// 原始语法文件
static const string grammar_file            = "../files/04-grammar.txt";
// 符合LL(1)文法的语法文件
static const string LL1grammar_file         = "../files/05-LL(1)_grammar.txt";
// 处理完成的语法文件
static const string processed_grammar_file  = "../files/06-processed_grammar.txt";
// 终结符，非终结符存储文件
static const string Vn_Vt_file              = "../files/07-Vn_Vt.txt";
// first, follow集存储文件
static const string first_follow_file       = "../files/08-first_follow.txt";
// 预测分析表
static const string pred_table_file         = "../files/09-predictive_analysis_table.txt";
// 分析栈
static const string stack_file              = "../files/10-stack_file.txt";
// 语法分析树
static const string tree_file               = "../files/11-grammatical_analysis_tree.txt";
static const string tree_dot_file           = "../files/12-grammatical_analysis_tree.dot";
// 符号表
static const string symbol_table_file       = "../files/13-symbol_table.txt";
// 语义动作分析
static const string semantic_analysis_file  = "../files/14-semantic_analysis.txt";





//过程名(待补充)
static const string period[period_num] = {
        "",                 //将0号位刨除，方便符合现实情况（从1开始）进行索引
        "预处理",           //第一个过程：预处理阶段（严格来说，属于词法分析阶段所需做的事情，此编译器将其分割）
        "词法分析",
        "语法分析",
        "语义分析",
        "中间代码生成",
        "优化"
};


//步骤名（待补充）
static const string step[step_num] = {
        "",                 //将0号位刨除，方便符合现实情况（从1开始）进行索引
        "去除注释",
        "去除空白",
        "生成词法单元序列",
        "文法处理——消除左递归，提取公共左因子",
        "文法处理——以' | '划分文法",
        "文法处理——求解 Vn Vt 集合",
        "文法处理——求 FIRST FOLLOW 集合",
        "文法处理——构建预测分析表",
        "预测分析（栈变化信息）",
        "生成语法分析树",
        "构建符号表"
};


//------------------------------------------------------------------------------


//基本结构体，枚举类型定义


//打开文件返回 类型
enum Return
{
    SUCCEED = 1,    //打开成功
    FAILED = 0      //打开失败
};


//------------------------------------------------------------------------------


//文件相关操作


//打开指定文件输入/输出
Return          input_file(fstream& inFile, const string& fileName);

Return          output_file(fstream& outFile, const string& fileName);


//关闭输入输出文件
void            close_file(fstream &file1, fstream &file2);


//输出分割线
void            divide_line();


//去除字符串前后多余的空格
void            remove_front_back_blank(string &str);

//字符串分割函数
vector<string>  split_string(const string& production);





#endif //FUNDAMENTALS_OF_COMPILING_HEADER_FILES_H
