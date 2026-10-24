//
// 创建者：86198；日期：2024/10/19。
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


#include "my_compiler/compilation_paths.hpp"
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
inline const string& source_file = my_compiler::legacy_compilation_paths().source_file;
// 去注释后的文件
inline const string& uncomment_file = my_compiler::legacy_compilation_paths().uncomment_file;
// 预处理完成的文件
inline const string& pretreatment_file = my_compiler::legacy_compilation_paths().pretreatment_file;
// 词法分析结果存储文件
inline const string& lex_file = my_compiler::legacy_compilation_paths().lex_file;
// 原始语法文件
inline const string& grammar_file = my_compiler::legacy_compilation_paths().grammar_file;
// 符合LL(1)文法的语法文件
inline const string& LL1grammar_file = my_compiler::legacy_compilation_paths().LL1grammar_file;
// 处理完成的语法文件
inline const string& processed_grammar_file = my_compiler::legacy_compilation_paths().processed_grammar_file;
// 终结符，非终结符存储文件
inline const string& Vn_Vt_file = my_compiler::legacy_compilation_paths().Vn_Vt_file;
// first, follow集存储文件
inline const string& first_follow_file = my_compiler::legacy_compilation_paths().first_follow_file;
// 预测分析表
inline const string& pred_table_file = my_compiler::legacy_compilation_paths().pred_table_file;
// 分析栈
inline const string& stack_file = my_compiler::legacy_compilation_paths().stack_file;
// 语法分析树
inline const string& tree_file = my_compiler::legacy_compilation_paths().tree_file;
inline const string& tree_dot_file = my_compiler::legacy_compilation_paths().tree_dot_file;
// 符号表
inline const string& symbol_table_file = my_compiler::legacy_compilation_paths().symbol_table_file;
// 语义动作分析
inline const string& semantic_analysis_file = my_compiler::legacy_compilation_paths().semantic_analysis_file;





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
