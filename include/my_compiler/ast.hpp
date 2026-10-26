#ifndef MY_COMPILER_AST_HPP
#define MY_COMPILER_AST_HPP
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace my_compiler::ast
{
    enum class TypeKind
    {
        Void, Bool, Char, Int, Float, Double, Structure, String, Pointer, Array
    };

    struct Type
    {
        TypeKind kind = TypeKind::Int;
        std::string tag;
        std::shared_ptr<Type> element;
        std::size_t extent = 0;
        static Type pointer(Type value)
        {
            Type result{TypeKind::Pointer, {}};
            result.element = std::make_shared<Type>(std::move(value));
            return result;
        }
        static Type array(Type value, std::size_t count)
        {
            Type result{TypeKind::Array, {}};
            result.element = std::make_shared<Type>(std::move(value));
            result.extent = count;
            return result;
        }
        Type decayed() const
        {
            return kind == TypeKind::Array ? pointer(*element) : *this;
        }
        bool integral() const
        {
            return kind == TypeKind::Bool || kind == TypeKind::Char || kind == TypeKind::Int;
        }
        bool operator==(const Type& other) const
        {
            return kind == other.kind && tag == other.tag && extent == other.extent &&
                   ((!element && !other.element) || (element && other.element && *element == *other.element));
        }
        bool operator!=(const Type& other) const
        {
            return !(*this == other);
        }
        bool floating() const
        {
            return kind == TypeKind::Float || kind == TypeKind::Double;
        }
        bool numeric() const
        {
            return kind == TypeKind::Bool || kind == TypeKind::Char || kind == TypeKind::Int || floating();
        }
    };

    std::string type_name(const Type& type);
    Type arithmetic_type(Type left, Type right);

    enum class ExpressionKind
    {
        Integer, Floating, Character, Boolean, Name, Member, Unary, Binary, Call, String, Cast, Address, Dereference, Index, Update
    };

    // 类型在语义阶段写回，后端只消费已经检查过的树，不再猜测表达式类型。
    struct Expression
    {
        ExpressionKind kind = ExpressionKind::Integer;
        std::size_t line = 0;
        std::int32_t integer = 0;
        double floating = 0;
        Type type;
        Type operand_type;
        std::size_t member_index = 0;
        std::string name;
        std::string text;
        std::string operation;
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> right;
        std::vector<std::unique_ptr<Expression>> arguments;
    };

    struct Initializer
    {
        bool list = false;
        std::unique_ptr<Expression> value;
        std::vector<Initializer> entries;
    };

    enum class StatementKind
    {
        Declaration, Assignment, Block, Conditional, While, For, Expression
    };

    struct Statement
    {
        StatementKind kind = StatementKind::Declaration;
        std::size_t line = 0;
        std::string name;
        Type type;
        std::unique_ptr<Initializer> initializer;
        std::unique_ptr<Statement> setup;
        std::unique_ptr<Statement> step;
        std::string operation = "=";
        std::unique_ptr<Expression> target;
        std::unique_ptr<Expression> value;
        std::vector<Statement> statements;
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> then_branch;
        std::unique_ptr<Statement> else_branch;
        std::unique_ptr<Statement> body;
    };

    struct Parameter
    {
        std::string name;
        std::size_t line = 0;
        Type type;
    };

    struct Structure
    {
        std::string name;
        std::size_t line = 0;
        std::size_t source_order = 0;
        std::vector<Parameter> members;
    };

    struct Function
    {
        std::string name;
        std::size_t line = 0;
        std::size_t source_order = 0;
        Type return_type;
        std::vector<Parameter> parameters;
        std::vector<Statement> statements;
        // 仍限定顶层末尾返回；void可return;或自然结束，不支持分支提前return。
        std::unique_ptr<Expression> returned;
        bool declaration_only = false;
        bool explicit_return = false;
    };

    struct Program
    {
        std::string source_file;
        std::vector<Structure> structures;
        std::vector<Function> functions;
        bool includes_stdio = false;
    };

    void check_names(Program& program);
    std::string dump(const Program& program);
    void write_text(const std::string& file, const std::string& text);
}
#endif
