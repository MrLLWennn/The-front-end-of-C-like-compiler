# 概述

## 2026-10-24最新有效安排：面试工程范围收口

用户明确纠偏：项目复盘优化服务于10月30日面试，重点是可写入简历的工程理解、技术栈应用和结果，不继续追求完整编译器。当前UR10在LLVM18.1.3 Debug配置11/11项CTest通过；新增分支语义24例、分支LLVM24例/320次调用，通过现有演示16组结果。已只读核对Oct24 17:51 CST日志及run.ZUmlwV实际AST/IR，并保存evidence/UR10_2026-10-24。O2本例生成select选1或2再加x；这不是运行加速测量。

从本轮起冻结编译器新增功能，必要新功能开发为0小时，预计本人整理/表达还需2—3小时。建议简历与主讲卡30分钟、两分钟项目讲述45分钟、六个关键工程追问45分钟、一个故障/工具串联30分钟，共约2.5小时；放进原预算剩余块，不重新追加。之后只修影响既定讲述/演示的问题，循环、调用、自定义Pass与重写前端均后置。后续“继续”默认围绕简历文字→证据→追问→工具/表达实践推进，不自动解释成扩语言指令。

当前主入口是[面试成果与项目讲述](/home/fridayyy/files/prepare/interview/2026-10/06_面试成果与项目讲述.md)，工作简历为resume/刘博文-联培面试简历-工程实践版-20261024.docx及PDF。简历采用工程实现、架构、验证措辞，保留受限子集和实际性能条件；过程标签省略，未增加独立原创、完整C或普遍提速主张。工程实现/执行收口与个人闭卷/独立掌握分开，不再把完整实现或逐函数背诵设为简历更新的无限前置。

原35h核心＋最多3h机动是10/23起的计划窗口，不代表现在仍剩35h；实际已用小时未知。内存池仅收口已有17项与Release对照，不开启UR03新接口；Hydra仅整理现有数据链/本人动作，工具用已有工程补弱点。10/26、10/28正式模拟和10/29 20:30材料冻结、10/30 14:20前到科研楼C区104A保持。以下旧“继续扩展/待11项运行”均为历史，以本段和06主讲卡为准。

## 2026-10-24本人继续：UR10受限条件分支待验收

用户能说明基本块/跳转与词法作用域的区别，并要求继续。补充运行时选路与前端合法性分析的区别后，本轮落实花括号if/可选else、嵌套分支、初始化状态独立分析与求交，以及LLVM比较/跳转/汇合。原Lexer、表驱动Parser和文法继续使用，旧9项测试保留，预计新增分支语义24例和分支LLVM24例，总11项CTest。新源码只经过静态审阅和Python语法检查，尚待本人构建、测试；UR09旧9/9不能替代新版本验收。

执行入口为compiler/docs/10_条件分支与初始化合流.md；新增演示在prepare/exercises/compiler/llvm-branches。本轮替换既有编译器拓展窗口，不另加时间，原35h核心＋最多3h机动、10/26与10/28模拟、10/29定稿及向量课程约束保留。实际已用时未知；若超窗口先顺延UR10、保留已验收UR09，不挤掉模拟。分支内return、循环和关系/逻辑运算尚未加入子集，不扩称完整C或自研优化器。

- [ ] 本人当前配置11项回归通过，确认旧9项无回归。
- [ ] 新语义24例与原生24例通过，检查目标诊断和实际运行结果。
- [ ] 给定演示与本人独立变式分别记录，讲解和材料生成不当作执行完成。

现有20261024简历与已验收条目保留；UR10运行通过之前不加入完成表述。以下UR09状态保留为历史证据。

## 2026-10-24 当前扩展：UR09局部变量与作用域（待本人执行）

旧UR08及风格/输出修订已有本人7/7回归和原生演示证据。本轮继续同一编译器：声明初值、简单赋值、嵌套词法块、初始化检查，以及入口块alloca/load/store到标准O2的衔接。代码及回归已编写，尚待用户构建；启用LLVM预计9项CTest，不把旧7/7算作本轮新功能通过。

当前机制、Pass/分析管理器说明、前后对比与用户命令见[UR09说明](docs/09_局部变量作用域与Pass管理.md)。原Lexer/Parser仍先运行；每块声明先于语句，函数仍要求唯一顶层末尾return。以下“初始子集”是UR08历史范围，不能与本轮待验范围混写。

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

## 2026-10-23：命令行与诊断工程化记录

前端支持显式路径：

```bash
my_compiler --input source.c --grammar grammar.txt --output-dir run-output
```

输入只读打开；驱动先检查文件和全部输出目标，缺文件或运行失败退出 1，无效参数与输入输出重叠退出 2。无参数保留旧 `../files` 布局。UR01 当时由用户运行 3/3 项 CTest 通过，覆盖帮助、参数、空格路径、输入保护、硬链接、缺文件与分进程隔离。

`CompilationPaths` 是单次运行的路径值，旧模块通过过渡适配器读取共享名字；独立进程可隔离，同一进程内多个编译实例仍不能并发运行。CLI 设计借鉴成熟工具的边界划分；该阶段尚未接入 LLVM。

UR02 对已检测的词法/语法错误抛出 CompilationError，由 main 输出英文诊断到 stderr 并退出 1，首次错误即停止，不发布本轮最终树和符号表。位置仍对应预处理文件；输出不构成跨文件事务，应使用新目录。首轮发现空输入被错误写入未初始化字节，随后修复成功读取条件，并以原始字节比较替代会转换换行的文本比较；用户随后报告 12 个诊断场景通过。不能由这些选定用例推断完整 C 语言正确性。

详细前后对比与问题见 `docs/04_cli_engineering.md` 和 `docs/05_diagnostics_engineering.md`；最新 LLVM 状态以下文为准。

## UR08：继续深化前端并实际接入LLVM（2026-10-23待用户验证）

UR01/UR02是第一轮工程基础，编译器仍会继续深化。按用户最新要求，当前主线为原Lexer/Parser→受限AST和名字检查→LLVM IR→O0/O2→原生目标文件→链接运行。内存池新增接口验收暂缓。新增--emit ast/llvm与可选MY_COMPILER_ENABLE_LLVM，LLVM18接口已按本机头文件静态核对；新代码没有运行，不宣称链路已接通。

完整函数变化、语义边界、所有权、验收和追问见[AST与LLVM最小编译链](docs/06_AST与LLVM最小编译链.md)。启用LLVM后预期7项CTest（含新增预测表回归），含AST 9场景及LLVM管线6场景；原4项保留。原模式仍默认启用；LLVM路径的int函数/参数/返回和加减乘只是明确定义的子集，不等于完整C兼容或全部优化已做完。

UR08首次用户构建已成功链接LLVM18.1.3，原4项回归通过；AST/LLVM测试在旧预测表的序列FIRST/nullable误判处失败。已修正表构造并增加compiler_prediction_table（4组文法/7格），启用LLVM后预期总7项；修复后待用户运行，尚未形成完整链路成功证据。

## 2026-10-23：终结符漏收集与编码统一（修复待本人运行）

本次用户构建成功，7 项 CTest 中 FOLLOW、预测表模块通过，另外 5 项在同一处 `Missing FIRST set for: continue` 失败。不是 5 个独立 LLVM 问题；这些测试使用完整文法，准备预测表时均被阻断。前一轮 4 份小文法/7 个表项覆盖了序列 FIRST 规则，但没有覆盖完整文法的符号收集，测试覆盖不足已补充。

`terminal_type_` 原本同时承担“展示类别”和“决定终结符成员”两项职责；表中漏了 continue，导致文法中虽然有 `<continue-stmt> -> continue ;`，终结符集合却没有 continue，FIRST 初始化也就缺项。不能把 `first_.find` 的失败改为空集合继续，那会把不完整状态伪装成合法预测表。

本轮 `cal_Vn_Vt_aux` 改为用流按空白读取右侧符号；按项目约定，尖括号包围的是非终结符，其余是文法终结符（包括 empty 记号）。类型表只提供中文标签，缺失时显示“文法终结符（未登记类型）”，不会排除符号。补上 continue 展示标签并清理重复 int/const 条目。此修复不等于给词法器新增 continue 或 char 的完整语言支持；回归夹具直接进入文法模块。

由此恢复应有不变量：每个收集到的终结符 t 都初始化 FIRST(t)={t}；求序列 FIRST 时依然严格检查前置数据。新增 continue、未登记 char、未登记 custom_token 三组回归，预测表测试现为 7 组文法/10 个表项，CTest 总数仍为 7。保留先前 4 组算法回归、9 个 AST 场景和 6 个 LLVM 端到端场景，必须一起通过后才可报告链路接通。

编码维护规则：本轮把维护中的 C++ 源码/头文件、脚本、说明、配置和网页示例统一为 UTF-8 无 BOM、LF。旧源码按 GB18030 严格解码后转存，原始字节已备份，不用替换字符吞掉损坏。构建目录和 files 中的历史输出保持原样；下次新运行将从 UTF-8 字符串生成新中间文件。`.editorconfig` 声明编码；编辑器仍以实际缓冲区检测结果为准，已有缓冲区需重新打开，不能仅靠更改显示编码重存旧乱码。

语言规则：命令行帮助、编译器报错与错误码保留英文，便于对照 LLVM 和工具链；函数原理注释、AST 展示、终结符类别和测试成功说明用中文；标识符、Token/探针协议、文法符号、LLVM IR 和测试名保留原样。AST 展示变更已同步严格断言，新日志、FIRST/FOLLOW、预测表加入 UTF-8 解码检查；未放宽正确性断言。

本轮仅完成文件编辑与静态核对，未由助手执行编译或项目测试。本人继续使用同一个 build-llvm 配置/构建/CTest 命令，当前不能声称 7/7 或 LLVM 运行通过。LibEdit/CURL 提示未阻止本次用户构建，与当前 FIRST 缺项不构成已证实因果。

## 2026-10-23：派生非终结符分类回归（本轮修复待本人运行）

用户再次构建成功，FOLLOW 和预测表 7 组/10 格模块通过；另外 5 项 CTest 失败。此次不再缺 continue 的 FIRST，而是在 `int a;` 上期待 `<var-declaration>'`、在函数上期待 `<func-declaration>'`。保留本次证据：minimal-our7jf4f、cli-7fi6zzig、diagnostics-5145846x、ast-ax7j4rwp、llvm-cc2pzjig。

这是上一轮修改引入的分类遗漏：判断“首字符 <、末字符 >”时没有考虑消除左递归/提取左因子后生成的 `<A>'`。它同时进入 Vn 和 Vt，导致 FIRST 初始化直接把它设置为自身，递归计算又以“已经有值”为由跳过展开。Parser 随后把这个内部非终结符当作源程序必须提供的词法符号，因而拒绝正常的分号或左花括号。07-Vn_Vt.txt 和 08-first_follow.txt 已实际核对到这一证据。

本轮分类改为 `productions_.find(symbol) != productions_.end()`。`cal_Vn_Vt()` 先调用 `generate_productions()` 读完全部产生式，再分类符号；因此右侧符号即便在后面才定义，也能从完整左部集合中查到。非终结符由文法定义决定，不再依赖尖括号或撇号的外观；类型表仍只负责展示，continue 和未登记终结符修复保留。

修改前：`symbol.front() == '<' && symbol.back() == '>'`。修改后：查找完整产生式表的左部键。对 `<A>' -> ;`，FIRST 应由产生式求得 `{;}`，不能是 `{<A>'}`。`Vn ∩ Vt = ∅` 是必须保持的不变量，新探针会对每组文法检查这一点。

在原 7 组/10 格基础上增加单撇号后定义、双撇号、可空派生前缀、无尖括号但有左部定义的 Tail、真正的小于号终结符共 5 组/6 格。现在预测表回归为 12 组文法/16 个表项；CTest 总数仍为 7。完整前端、AST 和 LLVM 的集成回归继续保留，必须由本人重跑。

这次主要修分类，不增加新语言功能，也不跳过 Parser。源码 UTF-8、英文诊断、中文注释与展示的规则保持。此前 FOLLOW、预测表成功说明及严格 UTF-8 读取已由用户实际运行通过；完整 LLVM 链路仍未取得通过证据。

## 2026-10-23 本人验收通过

用户已完成 Debug、LLVM18.1.3 启用构建，7/7 项 CTest 通过，0 项失败。已只读核对 LastTest.log 和真实生成产物：FOLLOW 5 场景、诊断 12 场景、预测表 12 组文法/16 格、AST 9 场景、LLVM 管线 6 场景均通过；旧最小程序、CLI 与输入保护回归保留通过。总测试时长 0.74s 仅是本次测试时长，不作为编译性能结论。

LLVM 证据目录为 build-llvm/test-runs/llvm-u4f4a9pg：真实源码生成 AST/IR/.o，由 clang 链接并运行；return42 程序返回42（编译器退出0）；O0/O2 的 simplify 在 -7、0、7、31 上均正确，O2 IR 将 x+0 消除；calc 的优先级结果正确，O2 实际将 y*2 改为左移一位。语义拒绝、输入输出重叠和失败产物检查通过。这里报告选定变换与运行正确性，不主张加速比例或完整 C 兼容。

UR08-A 执行验收通过；UR08-B 逐函数解释继续，UR08-C 本人表达式/故障变式待执行，整体任务未按测试通过直接勾完。新简历记录受限编译链和已通过回归，采用问题—实现—验证—结果表述；没有加入独立原创、自研 LLVM 优化器或新性能主张。原提交材料和旧工程更新版保留。

下一步说明及亲自操作入口见 docs/07_已验收编译链逐函数讲解.md。35h核心＋3h机动上限、10/26和10/28正式模拟、10/29定稿不变；不自动推进原P阶段或勾选G0/G1。

## 2026-10-24 代码风格与输出约定

新增C/C++遵循四空格缩进、结构块花括号独占下一行；.editorconfig与.clang-format记录后续约定。AST/语义/IR/优化/产物阶段继续使用原“过程/步骤”标题及分隔线。正常提示中文，诊断英文走stderr，机器协议字段保持。本人手动编译链run.wPVNA0已通过；本轮格式和日志修改待本人重建回归。当前语言子集及unique_ptr三次移动说明见docs/08_AST所有权与编码风格.md。


2026-10-24 UR09首次验证：原7项通过，新增两项在字符级左因子提取误拆identifier1后失败。本次改为完整文法符号分组和递归因子化，原始文法回归补至21组/37格；总9项CTest待用户重跑。原Lexer/Parser未被绕过。详见docs/09。
