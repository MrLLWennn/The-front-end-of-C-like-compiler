# 类C语言编译器

基于C++17的教学编译器。前端提供预处理、词法分析、表驱动语法分析、分层符号表与DOT语法树输出；受限AST路径进一步检查作用域、初始化和函数调用，并可接入LLVM18生成IR和原生目标文件。仓库附带具有main入口、函数调用、while循环和条件判断的简易C程序；新版样例还能包含本地头文件并调用printf输出结果；CP04加入多类型函数与结构体；CP05进一步支持固定数组、对象指针与for。当前前端12/12、LLVM配置19/19全量回归通过，包含与Clang的O0/O2运行对照。

## 支持范围

- legacy模式保留原有Token、文法中间结果和语法树输出。
- ast/llvm模式支持int、float、double、char、bool函数及参数；void返回函数允许末尾return;或自然结束。非void函数仍要求唯一的顶层末尾return。
- 支持整数、小数、科学记数法、ASCII字符字面量、数值转换、加减乘与六种比较；局部变量、成员赋值、词法块、带花括号的if/else和while保留。支持块内交错声明。
- 调用处必须有可见且相容的函数签名，支持原型、嵌套调用及自递归。检查参数数量、类型转换、局部遮蔽和使用前初始化。
- 顶层具名结构体可含基础类型或先定义的结构体成员，支持局部对象、嵌套成员读写、整值复制及同一翻译单元内按值传参/返回。初始化按叶成员追踪，不能因为写了一个成员就读取整个对象。结构体签名使用internal链接，不支持外部C聚合ABI对接。
- 本地头文件、包含保护、简单整数宏和受限stdio契约保留；新增stdbool.h的有限适配，不展开完整系统头文件。printf新增%f/%e/%g及固定精度，并正确提升float为double、char/bool为int；%s接受字符串字面量或char指针。
- 尚未提供完整C：不支持其他任意系统库接口、完整宏、const/volatile限定、函数指针、变长数组、全局对象、结构体花括号初始化/位域、通用外部聚合ABI、除法/取余、逻辑短路、break/continue、分支提前return或多源文件分别编译。
- int使用i32回绕加减乘；char为有符号i8，bool为真/假，float/double为相应LLVM浮点类型。数值窄化和浮点转整数只对结果可表示的输入作对照。while初始化检查保守保留入口，不证明循环终止。
- LLVM标准O2负责优化，目标后端生成.o，Clang负责链接。未实现自定义优化算法，未作性能测量。各层修改和边界见[CP04说明](docs/14_多类型函数与结构体.md)。

## 环境与依赖

Linux、支持C++17的编译器、CMake3.22及以上可使用显式配置命令。共享构建预设与workflow需要CMake3.25及以上；本轮固定Unix Makefiles生成器。测试需要Python3与Graphviz的dot；LLVM模式另需LLVM18开发包及clang-18。依赖由使用者准备，配置不会安装软件。

## 固定配置与验收入口

在仓库根目录运行：

```bash
cmake --workflow --preset frontend-check
cmake --workflow --preset llvm-check
```

| 工作流 | 构建目录 | LLVM | 当前注册测试 |
|---|---|---|---:|
| frontend-check | build-presets/frontend-debug | 关闭 | 12 |
| llvm-check | build-presets/llvm-debug | 启用 | 19 |

当前CP05前端12/12、LLVM配置19/19已通过；CP04的11/11、17/17及CP03的10/10、15/15作为历史记录保留。

每个工作流依次执行配置、构建、测试，各阶段使用同一个configurePreset。默认并行构建数2；若没有任何测试，测试阶段报错。可以用cmake --list-presets=all查看预设，或分别运行cmake --preset、cmake --build --preset与ctest --preset。

2026-10-24使用者已实际运行历史版本的两条预设工作流：前端8/8、LLVM18.1.3配置11/11通过。2026-10-25使用者运行CP02时前端8/9通过，新增程序测试在continue被误识别为标识符时失败，后续LLVM工作流因&&短路未执行。本轮补齐continue关键字并新增头文件/printf测试，当前注册10/15项；后续使用者已完成当前10/10与15/15全量回归，O0/O2的stdio演示均输出平方和55并退出0，与Clang对照一致。日志和产物已核对，完整验收归档在面试材料evidence/CP03_2026-10-25。两套配置有重叠。

个人路径和机器选项写入CMakeUserPresets.json，该文件已被忽略，不应提交。构建会生成compile_commands.json供编辑器工具使用。

## 显式配置方式

原来的调用方式仍可使用，例如：

```bash
cmake -S . -B build-llvm -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DMY_COMPILER_ENABLE_LLVM=ON
cmake --build build-llvm -j 2
ctest --test-dir build-llvm --output-on-failure
```

前端独立构建将MY_COMPILER_ENABLE_LLVM设为OFF。请让配置、构建和测试使用同一目录，避免读取另一份缓存或旧可执行文件。

## 运行

```bash
./build-presets/llvm-debug/my_compiler --help
./build-presets/llvm-debug/my_compiler     --input /path/to/source.c     --grammar /path/to/04-grammar.txt     --output-dir /path/to/new-output     --emit llvm --opt-level 2
```

将示例路径替换为实际文件。文法样例位于files/04-grammar.txt。--emit支持legacy、ast、llvm；优化级别支持0或2，且只用于llvm模式。无参数时兼容旧../files布局，推荐日常使用显式路径。

生成目录包含各阶段结果；受限AST为15-ast.txt，优化前后IR为16-before.ll与17-after.ll，目标文件为18-program.o。当前驱动不直接链接最终可执行文件。

## 编译并运行一个完整的简易C程序

在仓库根目录完成llvm-check工作流后运行：

```bash
python3 examples/run_mini_c.py build-presets/llvm-debug/my_compiler --opt-level 0
python3 examples/run_mini_c.py build-presets/llvm-debug/my_compiler --opt-level 2
```

默认演示[examples/mini_c_io.c](examples/mini_c_io.c)：包含stdio.h和本地mini_c_math.h，使用头文件中的整数常量及函数声明，计算平方和并真实打印`sum_squares(5) = 55`与`status: ok`。使用--example basic可改为原来的mini_c.c三个输入自检版本。演示脚本调用本编译器生成目标文件，再由clang-18链接为可执行文件，执行后应返回0；同时让Clang直接编译同一份源码作独立对照。每次运行在build-mini-c-runs下创建新目录，保存AST、优化前后IR、目标文件、两份可执行文件、日志和result.json。脚本不会替你构建、安装依赖或执行Git操作；任一步失败会以非零状态退出。

想亲自练习链接，也可以从本次output目录取18-program.o，运行`clang-18 /本次目录/output/18-program.o -o /本次目录/mini_c`，然后执行mini_c并检查退出码。main来自所选样例的C源码；Clang负责组织启动代码和系统运行库，LLVM目标后端负责生成.o，两者职责不同。

### 多类型和结构体样例（CP04已验收）

```bash
python3 examples/run_mini_c.py build-presets/llvm-debug/my_compiler --example typed --opt-level 0
python3 examples/run_mini_c.py build-presets/llvm-debug/my_compiler --example typed --opt-level 2
```

样例typed_structs.c及其本地头文件使用float/double、char、bool、void和结构体按值互调，预期输出value=3.00 grade=A accepted=1与score=3.50。每次均保留实际生成及Clang对照的结果；--example stdio/basic继续运行旧样例。

## 文件与失败处理

输入以只读方式使用，输出在指定目录生成；写输出前检查与输入的路径冲突及文件实体重合；新增预处理阶段先读齐依赖，再检查头文件是否与任何生成文件重合，包含硬链接。建议每次使用新输出目录。诊断发送到stderr：0表示本次编译流程完成，1表示检测到编译/文件等执行失败，2表示调用参数或配置不合法。

源位置目前对应预处理后的文件，未实现完整原文件位置映射。路径通过旧模块的共享适配状态传递，独立进程配合独立输出目录已测试；同一进程内并发多个编译实例不支持。

## 代码组织

| 位置或目标 | 职责 |
|---|---|
| compiler_core | 原前端、AST构建与语义检查，供入口和测试探针复用 |
| main.cpp、app/ | 参数、路径准备、模式选择和统一失败报告 |
| compiler_llvm、codegen/ | 可选LLVM接口、IR校验、标准优化与目标文件输出 |
| include/my_compiler/ | 新路径/错误/AST/后端接口 |
| front_end/、token/、SymbolTable/ | 既有前端实现与渐进适配 |
| tests/ | Python集成检查与C++文法探针 |
| docs/ | 对应工程改动和验证说明 |

## 当前验证与参考

历史LLVM18.1.3 Debug配置11项CTest通过，包含正常程序、FOLLOW、预测表、命令行、诊断、AST语义、局部变量与分支，以及O0/O2原生运行对照。分支语义24例，分支原生24例包含320次正例调用。测试通过只覆盖这些实现和用例，不构成完整C或全部输入正确性的证明。

本轮compiler_program_semantics通过39例；compiler_program_llvm通过62例，包括O0/O2下396次calc调用与两次完整程序运行。覆盖零次/多次迭代、嵌套控制流、六种比较、调用及递归、参数错误、未初始化和明确拒绝的功能。原生结果与Clang对照，求和案例另外核对独立数学公式；未测编译速度或生成程序性能。以上数量已与本轮results.json核对；它们是正确性检查，不是性能指标。

新增的compiler_headers_printf_semantics通过53例，compiler_headers_printf_llvm通过73例；覆盖头文件、宏、格式与参数、字符串保真、输入保护及O0/O2输出对照，本轮均已实际运行通过。

原前端参考《编译原理》。LLVM接入学习官方IR、可变变量、控制流与New Pass Manager接口；共享构建设置参考[CMake3.28 Presets手册](https://cmake.org/cmake/help/v3.28/manual/cmake-presets.7.html)。本轮配置讲解见[构建预设与重复验证](docs/11_构建预设与重复验证.md)，语义和分支说明见[条件分支与初始化合流](docs/10_条件分支与初始化合流.md)。

简易C程序扩展的边界、函数修改原理、验证和发布步骤见[从函数片段到简易C程序](docs/12_简易C程序与循环调用.md)。

头文件与printf的具体原理及支持边界见[头文件与printf](docs/13_头文件与printf.md)。

CP04新增71项前端用例与103项原生用例（含O0/O2、实际仓库样例、Clang双向标量ABI），使用者本轮已实际完成：前端11/11、LLVM17/17全量通过，71/103项类型用例记录及typed样例O0/O2输出已核对。没有性能计时结论。

第一次实际使用或编译自己的.c文件，请看[从源码到可执行程序](docs/15_实际使用与完整程序.md)。my_compiler输出.o，Clang驱动链接，最终程序再执行main；这三步的退出状态分别检查。


## CP05 数组 对象指针与for

支持固定长度及初始化推断长度数组、多维下标、嵌套花括号初始化与省略项补零、char数组字符串初始化；数组实参按外层维度退化为指针。对象指针覆盖多级指针、&、*、下标、->、参数/返回值、void*转换、同数组偏移/相减/比较，以及前后缀自增减。指针差在当前子集中返回i32，仅对差值可表示的输入验证；不宣称完整ptrdiff_t语义。

for生成初始化、条件、循环体、步进及出口控制流，初始化声明仅在该循环中可见。支持空子句及+=、-=、*=；仍要求花括号循环体，不支持break/continue、嵌套提前return。数组赋值、VLA、函数指针和括号声明器不在本轮范围。

新增测试在O0/O2下，将本编译器生成的目标文件链接运行，与Clang编译相同源码的结果及独立预期比较，包含非零退出码、零次循环、多维数组、指针ABI双向互调和预期失败诊断。数组越界、悬空/空指针解引用、别名后的完整初始化分析尚未实现；正例只使用有定义行为的输入。未测量性能。

```bash
cmake --workflow --preset frontend-check
cmake --workflow --preset llvm-check
python3 examples/run_mini_c.py build-presets/llvm-debug/my_compiler --example memory --opt-level 2 --output-root build-memory-demo
```

综合样例输出：`count=4 total=15.0 label=verified`。原理和限制见[数组、对象指针与for](docs/16_数组对象指针与for.md)。
