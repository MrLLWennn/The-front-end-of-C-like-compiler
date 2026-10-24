# AST所有权与当前编译边界

本轮来源：2026-10-24用户手动调用本项目编译器、查看AST/IR、以clang编译harness并链接目标文件，四组输入全部通过；产物为 /home/fridayyy/files/prepare/exercises/compiler/llvm-return/run.wPVNA0。属于给定材料的亲自操作，尚不作为自行设计变式的独立验收。

## 本次理解的校正

当前不是完整C编译器。原前端可识别的文法范围大于新AST/LLVM链可编译的范围。LLVM路径的函数返回值和参数为int，每个函数体只有一条return；支持多份函数定义，但当前没有源级函数调用、局部变量、指针、数组、结构体布局、控制流、完整预处理及标准库等完整C能力。当前二元运算是+、-、*；除法尚未加入AST优先级和后端处理。

自定义AST由我们写的llvm_backend::expression遍历，通过LLVM C++ API生成LLVM IR；LLVM不直接认识本项目的Expression结构。IR是带类型的SSA中间表示，包含函数/基本块等信息，与机器汇编并非一一对应。TargetMachine负责目标布局及后续原生目标代码生成，不属于源程序语法分析器。

原Lexer/Parser职责保留，但期间确实修复了错误传播、FIRST/FOLLOW、预测表和符号分类；“保留职责”不代表源码没改。legacy模式保留旧DOT/符号表路径；ast/llvm先经原Parser再构造AST，当前LLVM模式不额外生成旧DOT图。

O0/O2当前接的是LLVM标准管线；本项目工作为AST桥接、名字检查、IR构造、管线配置、驱动与测试。clang命令先编译harness.c，再组织链接器把harness目标、我们生成的.o、启动文件和必要库合为可执行文件。执行程序后，由系统和运行时走启动入口再进入main；Clang不是运行启动代码的那个进程。

## 三次移动必须结合树的生长看

```cpp
auto left = primary();
// ...识别运算符并构造右侧子树...
auto right = expression(priority + 1);
auto node = std::make_unique<Expression>();
node->left = std::move(left);
node->right = std::move(right);
left = std::move(node);
```

以x+2为例，初始left独占名字x节点，right独占整数2节点，node独占新建加法节点且两个孩子为空。

1. node->left = std::move(left)：由node的left成员接管x子树。局部left变为空指针，x节点地址不变。
2. node->right = std::move(right)：由node的right成员接管2子树。局部right变空，整数节点地址不变。
3. left = std::move(node)：让局部left接管完整加法树的根，node变空。下一轮再读到运算符时，left就代表已经构好的整个左表达式。

因此移动的是unique_ptr持有的管理责任，没有复制或搬迁树上的节点。若下一轮出现减法，新父节点再接管当前这整棵加法子树。树不被深拷贝，也始终只有一个拥有者负责释放每个节点。

std::move本身只把表达式转换为可用于移动的值类别；真正转移所有权的是随后调用的unique_ptr移动构造/移动赋值。单独写std::move(left)不会自行把left清空。普通命名变量是左值，unique_ptr禁止复制，故将命名局部指针交给拥有型成员时要明确std::move。

读取不转移：check_expression(*expression.left,...)借用const Expression&，根仍归原unique_ptr拥有。node->left = left不能复制unique_ptr。result.returned = expression(0)右边已经是返回的临时unique_ptr，会选择移动赋值，不必再套std::move。return left也允许返回值优化或隐式移动，不应机械改为return std::move(left)。

std::move不对所有类型都保证“变空”；这里unique_ptr不同对象之间移动后源指针为空有明确保证。移动string之后只能按该类型规则理解，不能套用指针结论。Builder构造函数file_(std::move(file))是在接管传值字符串的资源，不是在转移AST节点。

最外层Program被销毁时，函数的returned独占根释放，子节点的unique_ptr再递归释放各自子树；构建中途抛异常时，各局部拥有者同样清理已经构好的部分。

## 编码与输出规范

新增C/C++采用四空格缩进、结构块花括号独占下一行，控制关键字与左括号保持if()/for()习惯；函数/类型/命名空间展开，复杂续行合理缩进。纯格式整理先核对去除空白和注释后的词法单元序列，记录见本次工作区格式词法核对.json。其后新增的日志调用单独审阅。

新增AST、名字检查、IR、优化、中间结果和目标文件阶段沿用旧“过程：…----步骤：…”标题及相同分隔线。正常信息中文；编译错误保持英文及stderr，选项/错误码/IR/Token和探针协议保留原样。新增run_demo.bash仅由用户执行，给AST/IR展示、链接及运行也加同一框架；脚本不会自动构建或运行CTest。

本次只改格式与正常输出，没有扩展语言；修改后构建/7项CTest待用户执行，之前的7/7与手动练习通过保留历史证据。示例脚本是打印框架的使用入口，无需重新手敲所有命令。

参考：[C++草案unique_ptr移动赋值](https://eel.is/c++draft/unique.ptr.single.asgn)、[std::move定义](https://eel.is/c++draft/forward)、[Clang工具链](https://releases.llvm.org/18.1.8/tools/clang/docs/Toolchain.html)。
