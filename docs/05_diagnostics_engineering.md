# UR02 有原因的失败：从前端一直传到命令行

状态更新（2026-10-23）：用户定向执行compiler_diagnostics，12/12场景通过，证据目录diagnostics-lch6wibo。同一编译器构建的原3项CTest已在上一轮通过，之后仅修改Python测试，因此本轮所需回归已齐；这不是一次新的完整4/4运行记录。UR02-A执行验收通过，独立解释/变式另验。此前两次失败与修复保留在文末。预算沿用原UR02，不叠加。

## 面试中这项改动能回答什么

导师问“工具交给别人，错误输入会发生什么”：既要给人可读原因，也要给脚本可靠状态；错误分支不能继续构建看似成功的语法树。HR问“怎样验证改动”：本人执行正常/目标错误/邻近正常用例，并检查错误原因和产物，不把任意崩溃当作正确拒绝。这里是工程可靠性改进，没有速度提升或完整C编译器主张。

## 源码为什么要这样改

原有调用链是main→MyCompiler::compiling→FrontEnd→Lexer/Parser。多数中间函数返回void；Lexer一部分错误只写文件；Parser一部分错误直接exit、一部分回溯后继续；main可能继续走到return 0。仅在main追加一条打印不能补上这条失败链。

新增include/my_compiler/compilation_error.hpp，CompilationError继承std::runtime_error，将错误编号、对应文件、已有行列与原因格式化到what()。被修改的词法错误、Parser首个检测到的错误抛出它；异常沿调用栈回到main已有的catch(std::exception)，写stderr并return 1。main和上层void接口无需逐层改签名。未来若需要一次收集多条诊断、继续编辑器服务，可改为显式Result/诊断列表；当前一次CLI编译失败即结束，异常传播改动面较小。

LLVM Kaleidoscope教程的LogError返回nullptr，调用者检查空指针并向上传递失败；本项目借鉴“错误必须由调用方接住”的原则，具体采用C++异常，与教程机制不同，也没有集成LLVM库。[官方教程](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html)。

## 具体函数前后对照

| 文件/函数 | 原来 | 现在及理由 |
|---|---|---|
| lexer.h / tokenize_aux | 123abc记录错误后继续；未分类字节可能被忽略 | 123abc抛E_LEX_NUMBER，未支持字节抛E_LEX_CHAR；其他已计数错误在tokenize结束前抛E_LEX，阻止Parser接收无效Token流 |
| lexer.h / _remove_comment、_remove_blank | 失败时exit(1) | 抛runtime_error，由统一入口收尾，允许自动对象析构 |
| parser.cpp / Token分类 | 直接读i+1或i-1 | 先检查范围；末尾标识符和开头标识符不再越界 |
| parser.cpp / predictive_analysis_aux | input[next]等可能越界，空输入先读input[0] | 有界peek把输入尾部映射为结束标记；空输入给E_PARSE_EMPTY（本教学语言要求至少一条声明） |
| 同函数 / EOF | empty标志可能沿用上一个符号；输入耗尽时无条件记一次→empty | 每个剩余栈符号单独确认具有直接empty产生式，再弹出并记录；否则E_PARSE_EOF |
| 同函数 / 错误分支 | 回溯历史、丢弃输入、继续生成树 | 对终结符不匹配/无可用产生式抛E_PARSE_TOKEN/E_PARSE_TABLE，终止当前运行 |
| 同函数 / 结束判定 | 栈结束后未统一核对剩余输入 | 还有Token时E_PARSE_TRAILING，不能把有效前缀当完整输入成功 |
| parser.h / Stack::top/pop | 空栈可越界或exit(0) | 抛logic_error，作为内部不变量错误传播，不伪装成成功 |
| parser.cpp / generate_grammatical_analysis_tree_aux | 空推导可能下溢再下标访问 | 空推导提前报内部错误，循环用i+1<size避免size-1下溢 |
| app/driver_options.cpp | help只说明文件失败 | 补充已检测编译错误、stderr及预处理位置说明 |
| CMake/tests | 三项CTest | 增加compiler_diagnostics，复用现有Python解释器与目标路径表达式；原测试保留 |
| .gitignore | 只忽略build/ | 增加build-*/，避免build-engineering生成物进入Git暂存 |

核心变化（节选）：

```cpp
// 原来：i恰好是最后一个Token时，这次读取已经越界。
tokens_[i].is_identifier() && tokens_[i + 1].get_attribute_value() == "("

// 现在：&&从左向右短路，先证明下标有效，再读Token。
tokens_[i].is_identifier() && i + 1 < tokens_.size()
    && tokens_[i + 1].get_attribute_value() == "("
```

```cpp
// 现在：只要这里失败，就不会正常返回predictive_analysis。
fail("E_PARSE_TOKEN", "expected '" + top + "', got '" + symbol + "'", cur);

// main已有的统一出口：CompilationError也是std::exception。
catch (const std::exception& error) {
    std::cerr << "my_compiler: " << error.what() << '\n';
    return 1;
}
```

异常会越过Parser::grammatical_analysis中的生成树调用和FrontEnd中的生成符号表调用。文件流等自动对象随栈展开析构；这不等于自动回滚已经写出的预处理/Token文件。相比深层exit，调用方可以决定如何处理失败。

## 验收方法

用户执行，助手不代跑：

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake -S . -B build-engineering -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON &&
cmake --build build-engineering -j 2 &&
ctest --test-dir build-engineering --output-on-failure
```

预期4项CTest；首轮compiler_diagnostics内部8个场景（本次修复扩至12，见文末）：单条正确声明、两条相邻正确声明、缺分号、缺标识符、123abc、非法@、空输入、开头孤立标识符。正确场景要求0、stderr为空、树/DOT/符号表非空；错误场景要求明确1、对应编号和原因、原输入/文法不变，且全新输出目录里没有树/DOT/符号表。词法错误还要求没有Parser栈日志。SIGSEGV、超时或其他非零码都会让测试失败。

之后可运行`ctest --test-dir build-engineering -R '^compiler_diagnostics$' -V`查看每个场景和证据目录；这是看细节，不要求为了通过而反复重跑所有测试。

## 有意保留的边界

- 本次改为Parser遇到第一个已检测错误就停止；旧的尝试恢复行为不保留。若某个旧示例依赖恢复才能继续，它会被拒绝，需先判断是源程序错误还是原文法/选式问题。不能据此声称整个语言范围都已回归。
- 表构造、特殊选式和直接empty回退仍是旧课程实现，未消除所有文法冲突；未承诺捕获所有非法程序，也未完善语义分析。EOF仅沿用直接可空规则，间接可空链的通用处理后续另做。
- 行列来自02-pretreatment.txt。注释/空白预处理可能改变原源位置；诊断明确指向该文件，EOF不编造行列。原始源码的精确SourceManager映射未实现。
- 每次用全新输出目录；本轮失败不会生成后续产物，但复用旧目录时旧DOT/符号表不会自动删除。判断本次成功必须看本次退出码，不能只看目录里有文件。原子发布产物未实现。
- 多个进程可独立运行；UR01共享配置仍不支持同进程并发。异常保证已改路径的传播，不保证第三方/所有遗留内部错误都有同样的编号。
- 错误输出是文本协议，固定编号帮助测试和定位；尚未输出JSON诊断/完整多错误列表。

## 面试预测与参考回答

**导师浅问：为什么看到ERROR还不能判断程序失败？** 日志是文本，脚本通常看退出状态；原来可能打印后继续return 0。这轮将检测到的错误传播到main，同时提供stderr原因与非零状态，并阻止后续树生成。

**导师深入：为什么选异常，不逐层bool返回？** 现有多层void接口若改bool，需要每一层正确检查并转发；一次CLI运行失败就终止，用专门异常类型可集中处理，保留错误上下文。若需要可恢复、多诊断、禁用异常环境，则应使用显式Result/Expected和诊断收集，不声称异常永远最好。

**导师深入：throw后已写文件会还原吗？** 不会。RAII负责文件流等资源的释放，不自动撤销文件内容；本轮保留中间证据，用独立运行目录避免旧产物误认。完整事务输出需要暂存目录、成功发布和失败清理的单独设计。

**导师深入：为什么“非零即测试通过”不够？** 段错误、找不到文件也非零，但没有验证目标语法失败。用固定退出1＋错误编号＋目标原因＋后续产物未生成，才能较好地区分预期拒绝和测试环境失败。

**HR：这项优化是不是你的原创？** 编译器原版前端本人编写；此次借鉴成熟工具的错误传播思路，代码和测试由助手协助实现，本人执行并解释验收。实际新结果以用户本轮输出为准，不把参考原则称新算法。

## 检查清单与简历候选

- [x] UR01-A：此前用户Debug构建及3/3回归通过。
- [x] UR02-A：本轮诊断12场景通过；其余3项在此前同构建用户运行中通过，之后只改测试脚本。
- [ ] UR02-B：能说明一次缺分号失败如何从Parser传到main，以及为什么没有新DOT。
- [ ] UR02-C：本人给出并核对一个邻近输入变式；说明代码辅助范围。
- [ ] UR01-B/C/D：负例解释、配置边界、独立变式仍未单独验收。

待实际通过后简历候选：“为选定词法/语法错误补充诊断编号与失败传播，结合正常、异常输入及产物检查验证命令行行为。”当前先保留候选，未写成已完成结果；UR01已有运行证据可在下一次简历汇总时加入显式路径/输入保护等事实，不称LLVM集成。

## 2026-10-23 首轮回归发现的空输入缺陷与修复

用户实际运行结果：原3项CTest通过，compiler_diagnostics失败；其中valid、nearby_valid、missing_semicolon、missing_identifier、bad_number、unknown_character共6个场景通过，empty错误地得到E_LEX_CHAR byte 0，后面的leading_identifier尚未执行。不能记录整轮UR02通过。

只读检查本次产物diagnostics-abl58y6n/empty：source.c为0字节，01-uncomment.txt为0字节，02-pretreatment.txt却是b'\x00'。因此错误产生在空白预处理，不在Parser的E_PARSE_EMPTY判断。

remove_blank_aux原逻辑：

```cpp
char temp, pre;
file_in_ >> temp;                  // 空输入读取失败
while (temp == ' ' || temp == '\t' || temp == '\n')
    file_in_ >> temp;              // 纯空白到EOF后还可能一直循环
file_out_ << temp;                 // 读取失败也使用了未初始化/旧值
```

根因不是“空文件自带NUL”。char未初始化，失败的提取没有提供一个有效字符，随后使用它是不可靠的行为；这次观察到的是NUL，不保证每次都是NUL。只把temp初始化为0仍会向空文件伪造一个0字节，不能解决问题。

修复后以提取成功作为循环条件：

```cpp
bool has_content = false;
while (file_in_ >> temp) {
    if (!is_spacing(temp)) {
        has_content = true;
        break;
    }
}
if (file_in_.bad())
    throw std::runtime_error("Cannot read input file: " + from_file);
if (!has_content) {
    close_file(file_in_, file_out_);
    return SUCCEED;                // 预处理成功，输出仍为空
}
file_out_ << temp;                 // 已证明取得一个非空白字符
```

后续读取也改为while(file_in_ >> temp)，空白判断统一包含空格、制表、换行和回车。预处理允许空结果；“本教学语言需要至少一条声明”的判断仍由Parser负责，最终应为E_PARSE_EMPTY和退出1，而不是文件错误或非法NUL。真实磁盘读取badbit异常另行报读取失败。

诊断场景从8个扩为12个：增加纯空白、仅块注释、仅行注释、真实NUL字节对照。前三种预处理后应保持0字节并由Parser拒绝；真实NUL必须仍被Lexer拒绝，避免通过简单删除NUL来掩盖缺陷。E_PARSE_EMPTY用例增加“02文件必须0字节”的断言。原6个已过场景和正常邻近对照保留。

本轮尚未构建、执行测试；由用户执行：

```bash
cd /home/fridayyy/files/prepare/projects/compiler
cmake --build build-engineering -j 2 &&
ctest --test-dir build-engineering --output-on-failure
```

未改测试注册，无需重新配置CMake。预期仍4项CTest，新诊断测试内部12场景；若失败先看首个FAIL，不推进UR03。解释证据：用户已理解前端错误需要向命令行传递；准确说法是main中的catch(const std::exception&)捕获派生异常，输出stderr并返回非零。原日志仍可供人查看；本轮补的是脚本可检查的失败状态与后续产物约束。

面试追问：“为什么退出1还测试失败？”参考回答：这次虽退出1，却报错阶段和原因错误。通过中间产物逐阶段定位发现预处理伪造NUL，再修复成功读取的不变量，并补空白/注释及真实NUL对照；仅检查非零会漏掉这个缺陷。原前端与本轮辅助改动的贡献边界仍按事实说明。

## 2026-10-23 第二轮：CRLF文本读取导致的测试误报

用户再次执行：原3项CTest通过，诊断场景前7项（包括empty）通过，whitespace_only在输入保留断言处失败。只读核对diagnostics-17rwkjsw/whitespace_only/source.c实际字节为b' \t\r\n \t\n'，与写入内容完全一致；02-pretreatment.txt为0字节，stderr已为E_PARSE_EMPTY。之前预处理修复在这两个场景上有效，本次失败来自助手编写的测试。

原断言用source.read_text(encoding="ascii")与包含CRLF的原字符串比较。默认文本读取会规范化换行，实际读回字符串变成' \t\n \t\n'，造成误报。输入完整性应以文件字节作为比较对象，不能先做换行转换再断言字节保持不变。

```python
# 修改前
source.write_text(source_text, encoding="ascii")
require(source.read_text(encoding="ascii") == source_text, ...)

# 修改后
source_bytes = source_text.encode("ascii")
source.write_bytes(source_bytes)
require(source.read_bytes() == source_bytes, ...)
```

本轮仅改测试输入的写入/比较方式、讲解与进度记录，未改编译器。仍保留12个场景，CRLF和NUL对照都不放宽。块注释、行注释、真实NUL和开头标识符场景本轮尚未执行，UR02仍未通过。等待用户执行：

```bash
ctest --test-dir build-engineering -R '^compiler_diagnostics$' -V
```

Python脚本由CTest直接读取，不需要重新构建。其他3项已在本轮用户输出中通过，暂时只重跑修改涉及的诊断测试。只有目标测试实际通过后才更新验收，不能把只读定位或Python语法解析当作项目测试通过。

面试可讲的判断原则：测试也是软件；先比较原始字节和中间产物定位责任层，再决定修实现还是修断言。本例源文件未变、程序错误阶段正确，因此修正测试的比较语义，而不是让编译器绕过CRLF/NUL检查。

最终运行证据（2026-10-23）：用户执行ctest --test-dir build-engineering -R '^compiler_diagnostics$' -V，12场景全部PASS、目标测试1/1通过，0.08s仅为测试时长，无性能主张。材料中的“等待重跑”为历史状态，以本段与文首为准。


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
