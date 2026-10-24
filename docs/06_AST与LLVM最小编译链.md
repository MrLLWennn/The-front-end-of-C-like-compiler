# UR08 从原前端到AST、LLVM优化与目标文件

## 2026-10-24 当前扩展：UR09局部变量与作用域（待本人执行）

旧UR08及风格/输出修订已有本人7/7回归和原生演示证据。本轮继续同一编译器：声明初值、简单赋值、嵌套词法块、初始化检查，以及入口块alloca/load/store到标准O2的衔接。代码及回归已编写，尚待用户构建；启用LLVM预计9项CTest，不把旧7/7算作本轮新功能通过。

当前机制、Pass/分析管理器说明、前后对比与用户命令见[UR09说明](09_局部变量作用域与Pass管理.md)。原Lexer/Parser仍先运行；每块声明先于语句，函数仍要求唯一顶层末尾return。以下“初始子集”是UR08历史范围，不能与本轮待验范围混写。

2026-10-23依据用户明确纠正：编译器没有做完，继续深入同一项目并实际接入LLVM；暂停内存池UR03新增接口验收。UR01/UR02属于第一轮工程基础，不等于语义分析和后端已完成。本轮代码、测试已编写，尚待用户构建/执行；不能把本文件或源码生成当成接通证据。

## 当前深度与本轮范围

| 层次 | 已有/本轮状态 | 验收边界 |
|---|---|---|
| CLI、路径与输入保护 | UR01本人3/3通过 | 渐进配置适配器仍有进程内共享状态 |
| 已检测错误的传播 | UR02诊断12场景本人通过，原3项此前同构建通过 | 不是完整C错误覆盖；位置来自预处理文件 |
| AST、名字/受限类型规则 | 本轮编写待验证 | int函数/参数、返回整型表达式；不是完整语义分析 |
| LLVM IR、校验、O0/O2与.o | 本轮可选LLVM18模块，待验证 | 使用LLVM优化与原生目标后端，不是自研Pass或后端 |
| 原生链接与执行 | 用户运行的集成回归将调用clang-18并运行生成程序 | 测试将核对返回42及两类参数函数；尚无成功执行证据 |
| 后续深化 | 通过本轮再扩局部变量/作用域、赋值、if/while/基本块/phi、函数调用/ABI、源位置映射和产物发布 | 每项新需求必须先定义语义/负例，再实现和回归；不宣称这一轮终结项目 |

本机只读检查：/usr/lib/llvm-18/lib/cmake/llvm/LLVMConfig.cmake记录18.1.3，IR/Passes/Target头文件和LLVM导入共享目标存在；clang-18/opt-18/llc-18路径可见。未运行这些工具，未安装依赖。接口按本机LLVM18头文件核对，阅读用18.1.8同主次版本文档，不能把不同LLVM版本API混用。

## 同一项目怎么连接

```text
源文件
  → 原Lexer/Token（保留词素和行）
  → 原表驱动Parser按传入文法检查（typed模式不构造可视化树）
  → AST builder在同一Token流构造受限AST
  → 名字检查与受限int类型规则
  → --emit ast：15-ast.txt
  → --emit llvm：LLVM IR → verifier → O0/O2 → verifier → 原生.o
  → clang-18链接 → 运行并核对结果（由用户执行的测试脚本完成）
```

AST builder目前会对已通过原Parser的Token流再做一次受限结构识别；这是明确的过渡层，不是把原Parser替换掉，也没有假称已经在同一遍预测分析中完成AST构建。好处是先建立类型/语义表示与后端接口，代价是重复遍历和两处语法边界；后续可在原Parser匹配产生式时直接构建AST，届时再删除重复层。--grammar仍参与原Parser检查；换成任意文法不保证受限AST builder也支持。

旧可视化树面向产生式，叶子常是identifier/number这样的文法符号；LLVM需要具体变量名、常量和类型。新AST用unique_ptr单向拥有子表达式，保留具体词素，不建立父子shared_ptr循环。原可视化路径默认保留，LLVM路径不再构建它；不宣称已经修复全部旧树所有权问题。

## 初始语言与语义必须明确

- 函数定义的返回类型为int；参数为int列表、void或空列表。
- 函数体恰好一条return表达式；表达式支持十进制整数字面量、参数名、括号、二元加减乘。
- 当前正字面量范围0..2147483647；不把小数默默截断为整数。原文法不支持的形式仍由原Parser拒绝。
- 整数值按i32处理，加减乘定义为模2^32回绕；IR不添加nsw/nuw假设。这是当前C风格子集的明确规则，不声称完整C的有符号溢出规则。测试的C参考计算使用不会溢出的数值。
- 拒绝重复函数、重复参数、未声明名字；不支持的局部声明、函数调用、数组、结构体、隐式类型转换等明确报错，不静默丢掉。
- 当前名字检查只涉及函数和参数；没有承诺已经实现完整嵌套作用域、所有路径返回或全套类型推导。

## 文件/函数前后变化

| 位置 | 原来 | 本轮 |
|---|---|---|
| ast.hpp / ast_builder.* / ast.cpp | 无供代码生成使用的AST和名字检查流程 | Program/Function/Expression；具体词素、i32规则、duplicate/undeclared检查和可读AST输出 |
| FrontEnd::build_typed_program | 无 | 复用原Lexer及Parser，再构造/检查AST；不调用旧符号表文件重读路径冒充语义完成 |
| Parser::grammatical_analysis | 总生成可视化树 | 增加默认true的可选参数；typed路径只做语法检查，旧模式仍生成DOT |
| Parser语句列表分支 | 根据peek(next)是否为}决定是否继续 | 改为判断当前symbol；让当前的}正常结束语句列表，函数级用例覆盖这条前置 |
| llvm_backend.cpp | 无 | 独立LLVMContext/Module/IRBuilder、IR校验、PassBuilder O2、TargetMachine与目标文件输出 |
| driver_options/main | 只有原模式 | --emit legacy/ast/llvm；--opt-level 0/2仅用于llvm，禁用LLVM的构建明确拒绝llvm模式 |
| CompilationPaths | 13个生成路径 | 增加AST、前后IR和.o，全部加入原输入别名预检；不遗漏新产物的覆盖风险 |
| CMake | 原前端和4项回归 | AST属于core；LLVM可选独立目标，LLVM头文件不进入前端数据接口；开启时新6项CTest |

关键AST例子：

```c
int calc(int x, int y) { return x + y * 2; }
```

```text
函数 calc -> i32
  参数 x : i32
  参数 y : i32
  返回
    二元运算 + : i32
      名字 x : i32
      二元运算 * : i32
        名字 y : i32
        整数 2 : i32
```

树的分组体现乘法优先级；名字检查在生成LLVM Value之前拒绝y未声明等问题。后端只接收AST，不依赖原Token、DOT或具体解析表。LLVM Module拥有生成的函数/指令，LLVMContext活到Module销毁之后；裸Value*在这个受控寿命内借用，不用delete逐个释放。

## LLVM优化证据怎么判断

IRBuilder使用NoFolder，明确区分“建立原始IR”和“运行优化Pass”。对int simplify(int x){return x+0;}，预期优化前有add i32 %x,0；O0保留它，O2应简化为ret i32 %x。测试同时链接两个版本的.o并在-7、0、7、31上检查结果，避免只看IR文本就称正确。

verifyModule检查IR结构/类型等一致性，不证明源程序到IR的含义完全一致；因此还要AST形状、负例和原生执行结果。IR指令变少只说明选定冗余被消除，未测时间就不写运行速度提升。这里使用LLVM标准O2管线，不把LLVM的变换称作本人原创优化算法。

LLVM18的IR优化使用New Pass Manager；目标文件发射调用该版本TargetMachine的legacy PassManager接口。这是两段不同用途的API，不能因为名称含legacy就误以为本轮没用新优化管线。目标三元组/DataLayout从本机原生TargetMachine设置，只承诺本机链路；交叉编译需要另行配置和验证。

## 用户执行的一步

不需要先编译整个LLVM。使用已有LLVM18配置，单独构建目录：

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake -S . -B build-llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DMY_COMPILER_ENABLE_LLVM=ON \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm &&
cmake --build build-llvm -j 2 &&
ctest --test-dir build-llvm --output-on-failure
```

修复后预期7项CTest：原4项＋compiler_prediction_table＋compiler_ast_semantics（9场景）＋compiler_llvm_pipeline（6场景）。LLVM测试会用clang-18链接本项目生成的对象文件，并在临时目录运行生成程序，用户执行此命令即执行这些实践；日志和产物保留build-llvm/test-runs下。助手没有代跑。

首次配置会启用C语言，因为LLVM依赖探测可能编译C测试片段；本项目核心仍是C++17。若提示缺依赖或API错误，先给首个错误，先修本轮，不并行开内存池。不开LLVM的构建仍能使用legacy/ast，此时测试总6项，不能用它代替LLVM验收。

## 实际产物与后续手工实践

成功LLVM模式输出15-ast.txt、16-before.ll、17-after.ll、18-program.o。CTest的return42用例从真实源码生成.o，调用clang链接并期待程序退出42；编译器自身必须退出0，这两个退出码不能混为一谈。函数参数用例通过C harness核对调用约定与运算结果。

通过后再由用户亲自用CLI选择一个表达式变式，查看前后IR、用clang-18链接并运行。先使用自动回归定位环境/链路问题，再进行这项独立操作，不一次发多组新命令增加负担。

LLVM产物写出不是跨文件原子事务；语法/名字/IR校验失败时不发布新LLVM产物，但I/O或目标发射中途失败可能有部分文件。使用新的输出目录，并以本次退出码和证据判断。共享路径适配器仍限制同进程并发。没有加入多模块链接、调试信息、运行库input/output或JIT。

## 面试预测与参考答案

**浅问：以前已有语法树，为什么还要AST？** 原树反映推导过程，代码生成更关心具体名字、常量、运算和返回等语义结构；AST减少后端对文法重写细节的依赖，还要保留词素与明确类型。

**深入：LLVM能替你检查未声明变量吗？** 我们先在源语言层检查参数绑定等规则，再生成IR。LLVM verifier检查IR自身一致性，不能代替源语言语义分析，也不证明翻译完全正确。

**深入：优化前后怎样验证？** 保存前后IR，明确NoFolder构建与Pass变换的边界；O0/O2对象各自链接、运行同一组输入并核对结果。没有做公平计时，所以只报告所选冗余消除，暂不说加速百分比。

**深入：为什么不直接输出机器码？** 复用LLVM的优化和目标代码生成，把本人投入放在语言表示、语义、IR生成、驱动和验证；这叫集成LLVM后端，不称自研机器后端。

**深入：为什么还遍历两次Token？** 保留原Parser作语法闸门，以小改动先建立受限AST桥接；代价是重复结构识别与两处语法限制。后续把AST构造融合进产生式匹配，待行为回归齐全再移除重复层，未测不称前端加速。

**HR：本人做了哪些？** 旧前端本人编写；本轮结构和LLVM代码由助手协助实现和讲解，本人负责工具链操作、验收、结果判断和后续独立变式。源码已写时只能说待验证，不能提前声称已接通或独立掌握。

## 清单与后续阶段

- [x] UR08-A：本人 LLVM18.1.3 Debug 构建及7项CTest通过；预测表12组/16格、AST9场景、LLVM6场景，证据已保存。
- [ ] UR08-B：能对一个表达式解释Token、AST、名字检查、IR、优化、.o、链接、执行各阶段及所有权。
- [ ] UR08-C：本人选一项表达式或故障变式，先预测再运行，保存前后IR/退出状态与结果。
- [ ] UR08-D：只把实际通过部分更新简历，区分参考组件、助手实现与个人验证。

通过本轮后仍可深入：局部变量/作用域与alloca-load-store/mem2reg；if/while的基本块/分支/phi；函数调用与运行库ABI；原始SourceManager映射、一次遍历AST构建；性能对照与失效边界。按一个功能包一组验收继续，不把这些全部塞入面试前。

## 阅读只对准当前代码

[LLVM18 IR生成](https://releases.llvm.org/18.1.8/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html)、[New Pass Manager](https://releases.llvm.org/18.1.8/docs/NewPassManager.html)、[目标文件/链接](https://releases.llvm.org/18.1.8/docs/tutorial/MyFirstLanguageFrontend/LangImpl08.html)。每次只读对应函数，不另开完整LLVM课程。

## 2026-10-23首次用户构建：LLVM已编译，旧预测表阻止函数解析

用户实际配置、构建成功，LLVM18.1.3模块和my_compiler均已链接。原4项CTest通过；两个新测试在首个return42源码上同时失败，报E_PARSE_TABLE: no production for <mul_div_op> with ';'。AST场景与LLVM管线均尚未跑通，不宣称LLVM链路已完成。配置中的LibEdit/CURL缺失提示没有阻止配置与编译，不是当前失败根因。

源Token中42和分号正确，FIRST(<term>')是*/empty也正确；栈日志显示消耗42后，遇分号却将<term>'展开为<mul_div_op><factor><term>'。问题在generate_predictive_analysis_table_aux原来逐个遍历右侧符号的FIRST：后面的<term>'可以为空，就误把整个乘法产生式填进FOLLOW含分号的表格。开头<mul_div_op>必须消耗*/，所以整个右侧并不能为空。

修改前（核心逻辑）：

```cpp
for (const auto& symbol : right_symbols)
    for (const auto& token : first_[symbol]) {
        // 给每个token填整条产生式；任何一个symbol可空就引入FOLLOW。
    }
```

修改后直接复用此前FOLLOW修复所用的序列FIRST函数：

```cpp
const auto sequence = first_of_sequence(strings, 0);
for (const auto& terminal : sequence.terminals)
    append_unique(table[left][terminal], production);
if (sequence.nullable)
    for (const auto& terminal : follow_.at(left))
        append_unique(table[left][terminal], production);
```

序列FIRST只在前缀可空时继续看后一个符号；整个右侧全部可空才引入FOLLOW。不是屏蔽分号错误或跳过原Parser。保持遗留Parser的显式empty条目删除/直接空回退约定，未声称消除所有LL冲突；同时修正$列清理时边遍历同一vector边erase的失效风险。原特殊选式逻辑本轮保留，其完整通用化另验。

新增compiler_prediction_table直接读取4份小文法、核对7个格子：非空开头＋可空后缀不应借用FOLLOW；可空前缀＋必需终结符不能提前结束；整个右侧可空时应允许FOLLOW；后面的终结符不能冒充起始符。另有正确起始符的正对照。LLVM端到端测试继续保留，需两层证据一起通过。

修复后总7项CTest；用户重新配置、构建、测试。新增模块用例尚未执行。即便共享库编译和链接完成，也必须先修原前端的语法闸门，不能直接把LLVM阶段标记成功。

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
