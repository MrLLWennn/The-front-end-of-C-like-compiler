#ifndef MY_COMPILER_AST_HPP
#define MY_COMPILER_AST_HPP
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace my_compiler::ast
{
    enum class ExpressionKind
    {
        Integer,
        Name,
        Unary,
        Binary
    };

    // 子集只有有符号 i32 类型，加减乘按 32 位回绕。
    // 子节点由 unique_ptr 独占，不保留指向父节点的拥有关系。
    struct Expression
    {
        ExpressionKind kind = ExpressionKind::Integer;
        std::size_t line = 0;
        std::int32_t integer = 0;
        std::string name;
        char operation = 0;
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> right;
    };

    enum class StatementKind
    {
        Declaration,
        Assignment,
        Block,
        Conditional
    };

    // 声明可没有初值；赋值必须有值；块按源码顺序拥有内部语句。
    struct Statement
    {
        StatementKind kind = StatementKind::Declaration;
        std::size_t line = 0;
        std::string name;
        std::unique_ptr<Expression> value;
        std::vector<Statement> statements;
        // 条件分支独占条件表达式与两个块；else_branch 为空表示没有 else。
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> then_branch;
        std::unique_ptr<Statement> else_branch;
    };

    struct Parameter
    {
        std::string name;
        std::size_t line;
    };

    struct Function
    {
        std::string name;
        std::size_t line = 0;
        std::vector<Parameter> parameters;
        std::vector<Statement> statements;
        // 暂时仍要求函数顶层以唯一一条 return 结束，不处理分支返回。
        std::unique_ptr<Expression> returned;
    };

    struct Program
    {
        std::string source_file; // 记录预处理文件名，尚未映射到原始源码位置。
        std::vector<Function> functions;
    };

    void check_names(const Program& program);
    std::string dump(const Program& program);
    void write_text(const std::string& file, const std::string& text);
} // namespace my_compiler::ast
#endif
