# 概述

​	根据所学《编译原理》知识，主要目标在于实践编译器算法和提高工程实践能力，结合书上的理论以及网络资源对其中问题的相关解答，构建了C语言子集的编译器前端，核心实现：

* (1) **词法分析器**：基于DFA原理手写实现，完成预处理及Token化；(2) **语法分析器**：采用自顶向下的LL(1)预测分析法，实现了文法预处理模块，包括消除左递归与提取公因子、计算FIRST集与FOLLOW集，构建预测分析表；(3) **语法树与符号表**：构建可视化语法树，并设计层次化符号表管理多种符号类型。

* 以文件流形式进行中间结果输出:从源文件到语法树整个过程中间结果输出，便于验证各阶段是否正确：
  *  [00-source.txt](files\00-source.txt) ：源文件
  *  [01-uncomment.txt](files\01-uncomment.txt) ：预处理，去除注释
  *  [02-pretreatment.txt](files\02-pretreatment.txt) ：预处理，去除多余空格、制表符等
  *  [03-lexer.txt](files\03-lexer.txt) ：经过词法分析阶段所得Token流
  *  [04-grammar.txt](files\04-grammar.txt) ：自定义初始文法（为消除间接左递归对文法进行了预处理）
  *  [05-LL(1)_grammar.txt](files\05-LL(1)_grammar.txt) ：经过自顶向下的LL（1）文法处理
  *  [06-processed_grammar.txt](files\06-processed_grammar.txt) ：消除左递归、提取左公因子
  *  [07-Vn_Vt.txt](files\07-Vn_Vt.txt) ：确定终结符、非终结符集合
  *  [08-first_follow.txt](files\08-first_follow.txt) ：计算First、Follow集
  *  [09-predictive_analysis_table.txt](files\09-predictive_analysis_table.txt) ：预测分析表，推导
  *  [10-stack_file.txt](files\10-stack_file.txt) ：递归下降分析法所用到符号栈
  *  [11-grammatical_analysis_tree.txt](files\11-grammatical_analysis_tree.txt) ：生成语法分析树的结构（节点类型（内部、叶子）、编号、父节点类型及编号）
  *  [12-grammatical_analysis_tree.dot](files\12-grammatical_analysis_tree.dot) ：转为dot格式，便于生成可视化语法树
  *  [13-symbol_table.txt](files\13-symbol_table.txt) ：符号表
  * 待完成。。。。。



# 框架

总体：

Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----         2026/3/21      0:48                .idea
d-----        2024/11/19     22:46                back_end
d-----         2026/3/18     22:57                cmake-build-debug
d-----        2024/11/20     20:14                ErrorHandler                                                                                                                    
d-----         2024/12/6     10:12                files
d-----        2024/12/12     11:21                front_end
d-----         2024/12/2     12:13                SymbolTable
d-----        2024/11/24     22:08                temp_log                                                                                                                        
d-----        2024/11/22      8:54                token
d-----        2024/11/17     10:07                web visualization
-a----         2026/3/18     22:42            171 .gitignore                                                                                                                      
-a----         2026/2/27     20:57           1427 CMakeLists.txt
-a----         2024/11/4     12:44            299 compiler.cpp
-a----         2024/11/4     12:44            457 compiler.h
-a----         2024/11/8     18:46           1816 header_files.cpp                                                                                                                
-a----        2024/11/29     13:19           4321 header_files.h                                                                                                                  
-a----        2024/11/16     11:50            132 main.cpp                                                                                                                        
-a----         2026/3/20     14:57            418 README.md


    ------简易错误处理------- 


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        2024/10/31     12:36           1203 ErrorDefines.h
-a----        2024/10/31      0:12             75 ErrorTable.cpp                                                                                                                  
-a----        2024/11/20     20:14           5134 ErrorTable.h


    ------文件流输出------


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----         2024/12/6     10:12            412 00-source.txt
-a----         2026/3/18     22:57            322 01-uncomment.txt                                                                                                                
-a----         2026/3/18     22:57            265 02-pretreatment.txt                                                                                                             
-a----         2026/3/18     22:57           4487 03-lexer.txt
-a----         2024/12/4     20:46           2965 04-grammar.txt
-a----         2026/3/18     22:57           3474 05-LL(1)_grammar.txt
-a----         2026/3/18     22:57           4979 06-processed_grammar.txt                                                                                                        
-a----         2026/3/18     22:57           2386 07-Vn_Vt.txt
-a----         2026/3/18     22:57          24971 08-first_follow.txt
-a----         2026/3/18     22:57          68484 09-predictive_analysis_table.txt                                                                                                
-a----         2026/3/18     22:57          76551 10-stack_file.txt
-a----         2026/3/18     22:57          43859 11-grammatical_analysis_tree.txt
-a----         2026/3/18     22:57          60111 12-grammatical_analysis_tree.dot
-a----        2024/11/18     22:48        1758650 12-grammatical_analysis_tree.png
-a----         2026/3/18     22:57           4559 13-symbol_table.txt                                                                                                             
-a----        2024/11/26     23:28              0 14-semantic_analysis.txt


    ------词法分析器、语法分析器------


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----        2024/11/28     23:22                lexer                                                                                                                           
d-----         2026/3/18     23:04                parser
-a----         2024/12/5     20:33         116407 front_end.cpp
-a----        2024/12/12     11:21          16944 front_end.h
-a----         2024/12/3     10:53           1096 name_generator.h


    ------符号表------


Mode                 LastWriteTime         Length Name                                                                                                                            
----                 -------------         ------ ----
-a----        2024/11/30     15:37          12084 SymbolTable.cpp
-a----         2024/12/2     12:13          15729 SymbolTable.h                                                                                             


    ------词法分析器所生成的Token类别------


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----        2024/11/28     22:34                delimiter
d-----        2024/10/26     18:45                identifier
d-----        2024/11/28     22:44                keyword
d-----        2024/10/26     18:46                number
d-----         2024/11/1     10:09                operator                                                                                                                        
-a----        2024/11/22      8:54            765 token.cpp
-a----        2024/11/22      8:54           4275 token.h
-a----         2024/11/1     11:01            415 token_header.h

# 说明（待更新）

1、由于，对语法进行了限制，比如：不包括#include关键字、输出输入函数使用input()和output()，可以使用的关键字包括：

```c
    const string IF = "if";
    const string WHILE = "while";
    const string FOR = "for";
    const string RETURN = "return";
    const string BREAK = "break";
    const string ELSE = "else";
    const string VOIDD = "void";
    const string FLOAT = "float";
    const string INTEGER = "int";
    const string CASE = "case";
    const string SWITCH = "switch";
    const string CONS = "const";
    const string DOUBLE = "double";
    const string STRUCT = "struct";
    const string BOOL_ = "bool";
    const string DEFAULT_ = "default";

```

2、源文件示例：

```c
/* A program to perform Euclid's
Algorithm to compute gcd. */
    int gcd (int u,int v)
{  if (v == 0) return u ;
else return gcd(v,u-u/v*v);
/* u-u/v*v == u mod v */

}
/**/
void main(void)
{  int x; int y;
        x = input(); y = input();
       output ( gcd ( x , y ) ) ;
   x=1+1;
}

int d;
struct a
{
int aaa;
    double b(const int e)
    {
        int c;
        c = input();
    }
};
```

3、各部分知识点：

```c
//此头文件用于 编译过程的 符号表
/*
 * 符号表
 *
 * 1. 符号表是编译器其中一种重要的数据结构，程序中所使用的标识符的类型，作用域及其存储位置都保存在其中。
 * 2. 符号表是在编译器初期就已经建立。它根据语言规则识别标识符的正确使用，以便执行更高效的代码生成和错误检测，它具有内置的词法和语法分析阶段
 * 3.
 *      * 在分析阶段收集到的信息在语法分析阶段被编译器用来生成（中间）代码。
 *      * 被编译器用来实现编译期效率
 *      * 在如下多个阶段中使用：
 *          * 词法分析：在表中创建新的表条目，例如，token 的条目
 *          * 语法分析：根据 attribute type，作用域，维度，引用行，用途等信息
 *          * 语义分析：使用表中的可用信息检查语义，即验证表达式和赋值在语义上是否正确（类型检查）并相应地更新它
 *          * 中间代码生成：参考符号表，用于了解分配了多少和那种类型的运行时，并且有助于增添临时变量信息
 *          * 代码优化：使用表中的信息进行与计算机相关的优化
 *          * 目标代码生成：使用表中存在的标识符的地址信息生成代码
 * 4. 符号表条目——符号表中的每个条目都与在不同阶段的支持编译器的属性相关联
 * 5. 存储在符号表中的信息
 *      * 变量名称和常量
 *      * 过程和函数名称
 *      * 文本常量和字符串
 *      * 编译器生成的临时变量
 *      * 源语言中的标签
 * 6. 符号表的操作
 *      * allocate()        : allocate a new symbol table
 *      * free()            : remove all entries and free storage of symbol table
 *      * lookup()          : search for a name and return pointer to its entry
 *      * insert()          : insert a name in a symbol table and return a pointer to its entry
 *      * set_attribute()   : associate an attribute with a given entry
 *      * get_attribute()   : get an attribute associated with a given entry
 * 7. 谁来创建符号表的条目
 *      符号表条目是在分析阶段的词法分析器，语法分析器和语义分析器创建并使用的。我们采用 语法分析器 来创建的方法。
 * 因为语法分析器知道一个程序的语法结构，所以相对于词法分析器来说，语法分析器更适合来创建这些条目。它更好地区分一个标识符的不同声明。
 *      在更多情况下，词法分析器只能像语法分析器返回一个词法单元，比如 identifier ，以及指向这和此法单元的指针。
 * 只有语法分析器才能够决定是使用之前已经创建的符号表条目，还是为这个标识符创建一个条目
 * 8. 块的符号表的优化
 *      块的符号表的实现可以利用 作用域的最近嵌套规则。嵌套的结构可以保证可应用的符号表形成一个栈。
 * 在栈的顶部是当前块的顶部是一个当前快的符号表。栈中这个表的下方是包含这个块的各个块的符号表。
 * 因此，符号表可以按照类似于 栈 的方式来分配和释放
 *      有些编译器维护了一个散列表来存放可访问的符号表条目。也就是说，存放那些没有被内嵌块中的某个声明掩盖起来的条目。
 * 这样的散列表的实际上支持常量时间的查询，但是在进入和离开块需要插入和删除相应的条目。在从一个块 B离开时，编译器必须撤销所有因为 B 中的声明而对此散列表做出的修改。
 * 它可以在处理 B 的时候维护一个辅助栈来跟踪对这个散列表的所做的修改。
 *      语句块的 最近嵌套 规则：一个标识符的 x 在最近的 x 声明的作用域中。也就是说，从 x 出现的块开始，从内向外检查各个块时找到的第一个对 x 的声明。
 * */
```