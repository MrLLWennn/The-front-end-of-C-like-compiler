# UR09 局部变量、作用域与Pass管理

## 2026-10-24最新验收：UR09全套及原生演示通过

来源为本聊天用户提供的完整输出，已只读核对本机Oct 24 15:54 CST的LastTest.log及run.UXVQNz产物。LLVM18.1.3、Debug启用LLVM版本9/9项CTest通过：诊断14例、预测表21组文法/37格（含9组原始文法预处理）、原AST9例/LLVM6例，以及局部语义16例/局部原生14例。此前字符级因子提取及诊断契约两项修复均有本轮通过证据；1.59s只是测试总用时，不是性能结论。

本人执行现有llvm-locals演示，AST保留y声明/赋值，原始IR使用x/y槽位；O2后实际为shl x,1再add 2。目标.o由clang链接，四组输入-7/0/7/31得到-12/2/16/64，全部符合预期。证据另存prepare/interview/2026-10/evidence/UR09_2026-10-24，避免LastTest.log覆盖丢失。

- [x] 当前9项回归和给定材料原生演示已由本人执行通过。
- [ ] UR09逐函数理解/闭卷工程解释，依据本人回答另验。
- [ ] 本人自行选择的工程变式与目标负例，依据新证据另验。

本轮只收口执行状态及解释实际IR，不自动扩展控制流或函数调用。35h核心＋最多3h机动、10/26及10/28正式模拟、10/29定稿不变，实际耗时待本人记录；不勾选G0/G1。简历另存20261024版，写受限子集和已验收功能，不增加完整C、自研LLVM优化算法或速度主张。以下待重跑措辞均为历史状态，以本节为准。

2026-10-24：用户要求继续下一阶段，并解释Pass及New Pass Manager参考设计。本轮为原UR08编译器深化的后续小步，不叠加总预算；10/26、10/28模拟与10/29定稿仍保留。UR08旧版7/7和演示已由本人通过；本轮新增代码及9项总回归尚待用户执行。文件生成不是本阶段已实现成功的运行证据。

## 1. Pass到底是什么

Pass是对某一层IR执行一项分析或变换的处理模块，一条管线会组合多个Pass，也可能反复运行某些Pass；名称不保证实现只扫描一次。

| 角色 | 解决的问题 | 本项目的对应 |
|---|---|---|
| 分析 | IR具有什么性质，哪些变换成立 | LLVM计算函数/循环/调用关系等分析，结果可复用 |
| 变换 | 在保持既定语义下改写IR | 消除冗余、简化运算、把合适的局部内存访问提升为SSA值等 |
| PassManager | 按编排运行某层IR上的Pass | buildPerModuleDefaultPipeline返回模块级管线 |
| AnalysisManager | 按需计算、缓存及失效分析结果 | 本项目的loops/functions/calls/modules |
| PassBuilder | 创建标准管线并注册分析构造方式 | 本项目选择LLVM18的标准O2，而非自写每个变换 |

“运行了一个Pass”不保证改动了程序；不满足条件时可以保持原状。若前一步改变控制流，旧控制流分析可能失效；若只改变某个算术表达式，一些与控制流有关的分析仍可保留。变换通过PreservedAnalyses声明保留关系，LLVM框架管理失效，后续使用时按需重算。我们调用内置Pass，没有实现自己的失效协议。

阅读依据：[LLVM官方New Pass Manager设计说明](https://blog.llvm.org/posts/2021-03-26-the-new-pass-manager/)、[LLVM18使用说明的分析失效章节](https://releases.llvm.org/18.1.8/docs/NewPassManager.html#implementing-analysis-invalidation)。

## 2. 本项目具体参考了哪部分设计

直接对照[LLVM18默认优化管线示例](https://releases.llvm.org/18.1.8/docs/NewPassManager.html#just-tell-me-how-to-run-the-default-optimization-pipeline-with-the-new-pass-manager)，在codegen/llvm_backend.cpp的optimize()中：

1. 按推荐顺序声明Loop、Function、CGSCC、Module分析管理器，使相互引用的析构顺序正确。
2. 构造带TargetMachine的PassBuilder，让目标相关信息参与标准管线配置。
3. register*Analyses注册分析的构造方式；没有提前计算所有分析结果。
4. crossRegisterProxies连接不同IR层级的分析管理器。模块、调用图强连通分量、函数、循环并非四份互相隔绝的数据。
5. buildPerModuleDefaultPipeline(O2)取得LLVM标准编排，再由pipeline.run(module, modules)处理当前模块。

这里采用的是LLVM提供的API、层级组织与分析缓存/失效设计；没有重新编写New Pass Manager，也没有自行实现O2算法。此前x+0消除、乘二变左移只由前后IR观察到；未记录逐Pass前后变化，不能指定一定是某个Pass独自完成。多个Pass的前置简化也可能共同促成最终结果。

LLVM18的IR优化使用上述New Pass Manager，TargetMachine目标文件发射仍调用legacy::PassManager接口。两段用途不同，不能简单把legacy名称当成“整个编译器仍用旧优化器”。

## 3. 本轮动机及前后变化

此前Function只有returned，名字表直接把参数名映射到参数Value。可以翻译return x+1，却不能表达声明、多次赋值和词法块。

新增目标：

```c
int interview_calc(int x)
{
    int y = x + 1;
    y = y * 2;
    return y;
}
```

| 文件/函数 | 修改前 | 修改后 |
|---|---|---|
| files/04-grammar.txt | 声明只有无初值和数组形式 | 增加已有Parser所用identifier1 = expression的声明形式；原Parser仍先检查 |
| ast.hpp | Function只拥有返回表达式 | 增加Declaration/Assignment/Block语句及有序语句列表，保留末尾返回 |
| Builder::function/statement | 一进入函数体就要求return | 先读取局部声明、赋值、词法块，再读取顶层末尾return |
| check_statement/check_expression | 只检查参数名是否存在 | 作用域栈、同层重声明、查找、初始化与读取检查 |
| create_slot | 无 | 用临时Builder在入口块建立i32槽位 |
| expression(Name) | 直接返回参数Value | 找到槽位，用CreateLoad读取当前值 |
| statement(Assignment) | 无 | 计算右侧，再CreateStore写入目标槽位 |
| 参数处理 | 只保存Value | 把初始参数写入槽位，因此参数也可被赋值 |

AST仍由unique_ptr拥有子表达式，语句列表按值拥有只移动的Statement；LLVM只借用AST进行转换，不接管AST节点。

## 4. 为什么先生成alloca/load/store

LLVM指令结果采用SSA，每个结果只定义一次；可变内存位置允许多次写入。因此先把源码变量表示为槽位，再通过读写指令表达执行顺序。入口块分配让适用的LLVM提升优化更容易处理。

示意IR，实际名称和顺序以本次产物为准：

```llvm
%y.addr = alloca i32
%initial = add i32 %x, 1
store i32 %initial, ptr %y.addr
%old = load i32, ptr %y.addr
%next = mul i32 %old, 2
store i32 %next, ptr %y.addr
%answer = load i32, ptr %y.addr
ret i32 %answer
```

这里%y.addr这个SSA指针只定义一次，改变的是它指向的内存内容。未破坏SSA规则。O2可能用SROA、提升与后续简化移除不必要的内存访问；本项目没有硬编码“哪一个Pass必须做完全部工作”，也不承诺任意地址逃逸的变量都能提升。当前子集不支持取地址，因此本轮回归中的局部槽位适合提升。

参考[LLVM18教程第7章的可变变量及入口块alloca设计](https://releases.llvm.org/18.1.8/docs/tutorial/MyFirstLanguageFrontend/LangImpl07.html)。采用其内存表示思路，实际使用本项目i32规则、LLVM18 opaque ptr、原前端及New PM标准O2，没有照搬教程的double语言或旧式优化管线。

## 5. 作用域与失败语义

- 参数与函数最外层局部变量处于同一作用域；同层重声明报E_SEMA_LOCAL，重复参数仍报E_SEMA_PARAMETER。
- 进入嵌套花括号压入名字表，退出时弹出。查找从内向外，内层可以遮蔽外层；块内向未遮蔽的外层变量赋值会保留结果。
- 声明点先引入新绑定，再检查初值。因此int y=y会读取当前未初始化的y，不能偷用外层同名值。
- 无初值声明允许先定义后赋值；读取前未初始化报E_SEMA_UNINITIALIZED。赋值先检查右侧，再把目标记为已初始化。
- 声明初值中的前向引用和块外使用内层变量报E_SEMA_NAME。语义失败不发布AST、IR或目标文件。
- 当前只是没有分支的顺序状态检查；if/while加入后需要按控制流合并初始化信息，不能照搬一个布尔标记。

仍限int、加减乘、声明/简单赋值/嵌套词法块，函数最外层以唯一return结束；不处理嵌套return、分支、循环、函数调用、数组或完整C。原文法要求每个块的声明位于语句之前，暂不声称支持C99任意位置声明。声明与赋值是语句，尚不支持把赋值嵌入更大表达式或连等赋值。源位置仍指预处理文件。

## 6. 用户验收

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake -S . -B build-llvm \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DMY_COMPILER_ENABLE_LLVM=ON &&
cmake --build build-llvm -j 2 &&
ctest --test-dir build-llvm --output-on-failure
```

启用LLVM预计9项CTest：保留原7项，再加compiler_local_semantics（6正例＋10负例）和compiler_local_llvm（6场景各O0/O2＋2后端负例）。不开LLVM为7项，其中不包含原生代码验证。旧AST测试中“局部变量不支持”改为更具体的“局部变量已声明但未初始化读取”，保留原坏输入并强化目标语义检查，不删除负例。

新原生测试会比较实际生成的IR与.o，使用不溢出的C参考计算，在-7/0/7/31四组输入上核对，覆盖参数重赋值、内层遮蔽恢复、块内写外层及多层作用域。O0保留局部访问；O2检查本组适合提升的alloca/load/store被消除，但不按特定寄存器名或固定优化Pass顺序断言。

通过后运行完整可读演示：

```bash
bash /home/fridayyy/files/prepare/exercises/compiler/llvm-locals/run_demo.bash 2
```

会展示声明/赋值AST、原始读写IR、优化后IR及四组实际结果。当前脚本和测试由用户执行，助手不代跑。测试通过之前不更新简历为已完成新功能，也不把代码生成记作本人独立掌握。

## 7. 导师可能追问及参考回答

**你新增的功能和LLVM提供的优化分别是什么？** 前端增加语句表示、作用域与初始化检查，后端增加可变变量的内存形式翻译；LLVM完成标准O2中的提升和简化。没有自研优化算法的主张。

**有了SSA，为什么还能反复给y赋值？** SSA限制IR计算结果的定义；%y.addr指针不变，store改变其指向的内容。之后LLVM可将合适的内存访问改成SSA值。

**作用域退出为什么没有调用free？** 当前i32局部槽位是函数入口alloca，名字可见性与底层存储寿命分开；弹出名字表即可恢复外层绑定，槽位由函数执行栈/后端管理。未来加入需要析构的对象时另设生命周期规则，不能只弹名字表。

**O2移除内存访问就一定更快吗？** 这是IR优化与功能正确性的证据。最终机器码和运行耗时仍需另行对照测量。

**分析结果为什么需要失效管理？** 变换会改变分析所依据的IR事实；继续复用过期结果可能导致错误判断。保留仍有效的分析可避免重复计算，失效部分在下次请求时重算。


## 2026-10-24：初始化声明触发的文法预处理修复（待本人重跑）

本人构建和链接成功，原7项通过，新局部语义/原生两项均失败在原Parser；未执行到新的LLVM代码。实际05文法把identifier1拆成identifier及尾部1，出现 `<var-declaration>' -> ; | [ number ] ; | 1 = <expression> ;`。这证明错误在提取公共因子阶段，不是通过把identifier1强行当identifier匹配就能修好。

旧findLongestCommonPrefix按字符截短字符串。新实现先把右部拆成完整文法符号，按首符号分组，再比较组内符号序列并递归处理余项；新非终结符避让原文法和已生成名字。空后缀明确输出empty。对原三个声明选择，目标结构为：

```text
<var-declaration> -> <type-specifier> <var-declaration>'
<var-declaration>' -> identifier <var-declaration>'' | identifier1 = <expression> ;
<var-declaration>'' -> ; | [ number ] ;
```

这保留普通声明、数组声明语法和初始化声明各自的完整符号边界。原直接左递归分支沿用，但新建名字也使用避重名函数；没有据此声称已经实现任意CFG的完整LL(1)转换或解决原Parser全部特殊选式。

测试探针增加--raw模式，先从04做实际预处理再生成表。原12组/16格全部保留，新增9组/21格，当前预计合计21组文法/37格。新增覆盖identifier/identifier1、带共同类型前缀、带撇号非终结符、+与+=、完整选择式作为前缀的empty、多空格/制表符、部分分组及数组尾部、派生名冲突。总CTest仍9项，修复尚未由助手构建或运行。

复跑命令：

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake --build build-llvm -j 2 &&
ctest --test-dir build-llvm --output-on-failure
```

## create_slot分配发生在哪一阶段

编译时create_slot通过CreateAlloca建立的是一个LLVM IR指令对象；还没有为将来运行的函数取得真实局部变量地址。生成程序运行时，保留下来的alloca表示当前函数栈帧中的存储，函数返回时自动释放。本实现把分配放在函数入口，因此退出内层花括号只撤去名字绑定，不单独释放栈槽。

O2可以把合适的alloca/load/store消成SSA值，所以最终机器码未必还有该槽位。SSA值也不保证一一对应物理寄存器，最终位置由后端决定。分析和变换的职责边界与用户NewPM总结一致：PassBuilder组装标准管线，分析管理器按需计算、缓存及失效分析结果。

语义依据：[LLVM18 alloca说明](https://releases.llvm.org/18.1.8/docs/LangRef.html#alloca-instruction)。


## 2026-10-24：局部变量已执行通过，补稳定诊断（待本人重跑）

本轮构建成功，8/9 CTest通过。符号级因子修复的21组文法/37格、16项局部语义、14项局部原生用例及旧LLVM管线都通过；剩余compiler_diagnostics在int ;上提前停止，不能把后续未运行诊断算通过。

int ;仍返回失败、没有发布最终产物，但报错从E_PARSE_TOKEN（缺identifier）变为E_PARSE_TABLE（派生规则没有选择）。因子化前栈顶直接是identifier；因子化后栈顶成为可选identifier/identifier1的非终结符，所以终结符不匹配分支未触发。语言接受性正确，公开诊断契约发生回归。

本次只改无合法选择且不能空推导的报错路径：从当前预测表行收集有效的前瞻Token，将Parser使用的identifier/identifier1/identifier-function/identifier-struct归并为用户可读的identifier标签，排序去重。如果预期唯一，报告E_PARSE_TOKEN及具体期待；预期不唯一则保留E_PARSE_TABLE和原no production信息。该归并仅用于显示，不改变Token匹配、选式或identifier1的语法职责，也不把所有缺表错误都误称缺标识符。

不硬编码<var-declaration>'的名字，不允许错误输入继续执行，不修改原missing_identifier断言。补上初始化前缺名字与函数内缺名字两例；原12个诊断用例保留，当前14个待重跑。总CTest仍9项。

实际initialized_O2的原始IR包含x/y的alloca及load/store；优化后为shl x,1再add 2，等价于2*(x+1)。已由本人原生调用回归验证四组输入，未据此声称运行加速或独立实现LLVM变换。

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake --build build-llvm -j 2 &&
ctest --test-dir build-llvm --output-on-failure
```

新诊断修订只经过静态代码及Python语法检查；助手未运行构建、测试或演示。完整9/9仍待本人确认。

## 8. 用这次真实IR讲清函数实现与工程取舍

本节对应本人run.UXVQNz，源码为int interview_calc(int x)，先int y=x+1，再y=y*2，最后return y。

**create_slot的职责。** 临时IRBuilder的插入点是函数entry.begin，只插入alloca；主Builder继续按源码顺序插入store、load和运算。这让槽位集中在入口且不打乱初始化/赋值次序。创建IR时并没有为未来函数调用分配真实运行栈内存：CreateAlloca先创建指令对象，目标函数执行时保留下来的alloca才表示运行栈存储，O2也可能将其消除。因为每次在入口最前插入，本次y.addr出现在x.addr前面；两个槽位建立的顺序不改变后续store的语义。

**为什么参数x也有槽位。** 旧版直接将名字映射到LLVM Argument，适合只读参数；本轮允许x=x+1，因此参数和局部变量统一映射到AllocaInst*。函数入口先store原参数，后续名字读取由expression(Name)调用CreateLoad，声明/赋值由statement先求右侧表达式再CreateStore。即使本例x没有重赋值，也先使用这一统一策略。代价是O0多了内存访问和IR指令，收益是前端翻译与可变变量规则简单一致；O2再移除适合提升的访问。

**SSA和可变变量没有矛盾。** %y.addr只定义一次，指向的内存内容可以经store改变；%y.value和%y.value1是两个不同的读取结果。不能把同一个%y重复定义来模拟源码赋值。也不必在前端立刻手写完整SSA构造算法。

本次优化后的实际函数主体：

```llvm
%add = shl i32 %x, 1
%mul = add i32 %add, 2
ret i32 %mul
```

数学上对应2*(x+1)=2*x+2，本项目还明确采用无nsw/nuw的i32回绕规则。这一例x/y槽位和load/store全部消失，不能推广为任意取地址或逃逸变量都能消除。%add/%mul只是IR名字，不约束必须是加法/乘法，也不是物理寄存器。现在能确认整体O2产生这些变换，没有逐Pass跟踪，不能断定全部出自一个指定Pass；没有基准计时，不能由IR更短推出运行快几倍。

**作用域为何独立于栈槽。** 语义层维护Binding{initialized}作用域栈，代码生成层维护名字到AllocaInst*的作用域栈。进入块压表、退出块弹表，查找从内向外。遮蔽时inner y与outer y指向不同槽位，退出内层恢复外层；名字不可见不等于需要立即free槽位。当前不涉及带析构的对象，也没有分支初始化合并。

**两次失败为什么值得作为面试工程案例。** 第一次不是LLVM不支持赋值，而是原预处理将identifier1按字符切成identifier和1；通过查看05文法定位，改用完整符号序列比较，并从04原始文法开始做回归，防止只测加工后的输入。第二次不是int ;被接受，而是正确因子化改变Parser栈顶，原E_PARSE_TOKEN变成E_PARSE_TABLE；从有效预测表项推导用户可读的唯一预期Token，保留原断言并加邻近负例。标识符类别只在诊断展示中归并，不修改真正匹配规则。

### 针对导师与HR的预测问答

这些是根据项目内容推测的追问，不代表29所公布题目。

**为什么不一开始直接生成最简IR？** 先保证声明、赋值、作用域和错误语义可表达、可测，再复用成熟LLVM优化；避免把语义实现与手工简化耦合。这个选择付出O0冗余，换取实现清晰和后端复用。

**怎么验证编译器正确，而不只是构建成功？** 模块级检查文法和目标语义负例，端到端对O0/O2目标文件分别链接执行；同输入对照预期结果，并检查目标诊断、失败产物和输入不变。verifier只检查IR结构/类型等约束，不能替代源语义和运行结果验证。有限测试仍不代表支持完整C。

**你的工程工作和LLVM分别承担什么？** 本项目负责原前端、AST与局部语义、作用域/初始化规则、IR生成、可选后端与驱动，以及目标错误和端到端回归；参考LLVM官方可变变量教程的入口槽位思路，调用标准New PM O2及目标后端，不把LLVM内置优化当自写算法。

**改进最能说明什么能力？** 可以讲从失败日志和中间文法缩小根因、修复符号边界，再用原始输入及邻近负例守住接口契约的过程。用具体定位与结果说明，不用“掌握完整编译器”替代证据。若被问参考/工具参与，按实际过程回答。

### 自查与下一步

沿用CR01：AST独占与LLVM借用寿命；CR02：名字存在还需初始化、可见域及整数规则；CR03：失败必须命中目标诊断，原始文法到结果需要端到端覆盖；CR05：前端统一内存表示与LLVM提升的取舍、IR变化与实测性能的区分。0分不会、1分看答案懂、2分闭卷解释并举例、3分自行设计变式并有证据。此次运行只更新执行项，不擅自给自查加分。

后续工程变式优先选择一项自己预判结果的遮蔽/初始化负例并检查诊断与产物，或将本次失败到修复整理为两分钟说明；不重复九项已过测试，不立即开启完整if/while、调用和全C。需新增功能时在原预算内替换，并保留两轮正式模拟。

参考：[LLVM18可变变量教程](https://releases.llvm.org/18.1.8/docs/tutorial/MyFirstLanguageFrontend/LangImpl07.html)、[alloca语义](https://releases.llvm.org/18.1.8/docs/LangRef.html#alloca-instruction)。
