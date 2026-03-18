//
// Created by 86198 on 2024/10/30.
//

#ifndef MY_COMPILER_ERRORDEFINES_H
#define MY_COMPILER_ERRORDEFINES_H



namespace my_compiler_error_handler
{

    enum ErrorDefines
    {
        // *** 期望 ***
        EXPECTED = 1,
        // *** 缺失 ***
        MISSING = 2,
        // *** 冗余 ***
        REDUNDANT = 3,
        // *** 重声明 ***
        REDECLARED_IDENTIFIER,      /*重复声明的标识符*/
        REDECLARED_PROCESS,         /*重复声明的过程*/
        // *** 未声明 ***
        UNDECLARED_IDENTIFIER,      /*未声明的标识符*/
        UNDECLARED_PROCESS,         /*未声明的过程*/
        // *** 未定义 ***
        UNDEFINED_PROCESS,          /*未定义的过程*/
        // *** 不合法 ***
        ILLEGAL_DEFINE,             /*不合法的定义*/
        ILLEGAL_WORD,               /*不合法字符*/
        ILLEGAL_RVALUE_ASSIGN,      /*不合法的右值赋值*/
        // *** 不兼容 ***
        INCOMPATIBLE_VAR_LIST,      /*不兼容的变量列表*/
        // *** 不匹配 **
        NO_MATCHING_FUNCTION,       /*没有匹配的函数*/
        // *** 检测到语法错误，但是不清楚错误类型 ***
        EXPECTED_UNQUALIFIED_ID
    };

}




#endif //MY_COMPILER_ERRORDEFINES_H
