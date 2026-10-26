#include "my_compiler/ast_builder.hpp"
#include "my_compiler/compilation_error.hpp"
#include "my_compiler/stage_output.hpp"
#include <charconv>
#include <cmath>
#include <system_error>
#include <utility>

namespace my_compiler::ast
{
    namespace
    {
        using my_compiler_token::Token;
        class Builder
        {
        public:
            Builder(const std::vector<Token>& tokens, std::string file)
                : tokens_(tokens), file_(std::move(file))
            {
            }

            Program run()
            {
                Program result;
                result.source_file = file_;
                std::size_t order = 0;
                while(position_ < tokens_.size())
                {
                    if(peek() == "struct" && peek(2) == "{")
                    {
                        auto declaration = structure();
                        declaration.source_order = order++;
                        result.structures.push_back(std::move(declaration));
                    }
                    else
                    {
                        auto declaration = function();
                        declaration.source_order = order++;
                        result.functions.push_back(std::move(declaration));
                    }
                }
                if(result.functions.empty())
                    fail("E_AST_EMPTY", "expected a function definition");
                return result;
            }

        private:
            const std::vector<Token>& tokens_;
            std::string file_;
            std::size_t position_ = 0;

            std::string peek(std::size_t distance = 0) const
            {
                return position_ + distance < tokens_.size()
                    ? tokens_[position_ + distance].get_attribute_value() : "<eof>";
            }

            std::size_t line() const
            {
                return position_ < tokens_.size() ? tokens_[position_].get_line() : 0;
            }

            [[noreturn]] void fail(const std::string& code, const std::string& text) const
            {
                throw CompilationError(code, file_, line(), 0, text);
            }

            void expect(const std::string& text)
            {
                if(peek() != text)
                    fail("E_AST_EXPECTED", "expected '" + text + "', got '" + peek() + "'");
                ++position_;
            }

            std::string identifier()
            {
                if(position_ == tokens_.size() || !tokens_[position_].is_identifier())
                    fail("E_AST_NAME", "expected an identifier, got '" + peek() + "'");
                return tokens_[position_++].get_attribute_value();
            }

            static bool type_start(const std::string& value)
            {
                return value == "int" || value == "char" || value == "bool" ||
                       value == "float" || value == "double" || value == "void" || value == "struct";
            }

            Type type()
            {
                const auto value = peek();
                ++position_;
                if(value == "int")
                    return {TypeKind::Int, {}};
                if(value == "char")
                    return {TypeKind::Char, {}};
                if(value == "bool")
                    return {TypeKind::Bool, {}};
                if(value == "float")
                    return {TypeKind::Float, {}};
                if(value == "double")
                    return {TypeKind::Double, {}};
                if(value == "void")
                    return {TypeKind::Void, {}};
                if(value == "struct")
                    return {TypeKind::Structure, identifier()};
                fail("E_AST_TYPE", "expected a supported type, got '" + value + "'");
            }

            Structure structure()
            {
                Structure result;
                result.line = line();
                expect("struct");
                result.name = identifier();
                expect("{");
                while(peek() != "}")
                {
                    if(position_ == tokens_.size())
                        fail("E_AST_STRUCT", "unterminated struct definition");
                    Parameter member;
                    member.line = line();
                    member.type = type();
                    member.name = identifier();
                    expect(";");
                    result.members.push_back(std::move(member));
                }
                expect("}");
                expect(";");
                return result;
            }

            Function function()
            {
                Function result;
                result.line = line();
                result.return_type = type();
                result.name = identifier();
                expect("(");
                if(peek() == "void" && peek(1) == ")")
                    expect("void");
                else if(peek() != ")")
                {
                    while(true)
                    {
                        Parameter parameter;
                        parameter.line = line();
                        parameter.type = type();
                        parameter.name = identifier();
                        result.parameters.push_back(std::move(parameter));
                        if(peek() != ",")
                            break;
                        expect(",");
                    }
                }
                expect(")");
                if(peek() == ";")
                {
                    expect(";");
                    result.declaration_only = true;
                    return result;
                }
                expect("{");
                while(peek() != "return" && peek() != "}")
                {
                    if(position_ == tokens_.size())
                        fail("E_AST_RETURN", "expected a final return expression");
                    result.statements.push_back(statement());
                }
                if(peek() == "return")
                {
                    expect("return");
                    result.explicit_return = true;
                    if(peek() != ";")
                        result.returned = expression(0);
                    expect(";");
                }
                else if(result.return_type.kind != TypeKind::Void)
                    fail("E_AST_RETURN", "expected a final return expression");
                expect("}");
                return result;
            }

            Statement statement()
            {
                Statement result;
                result.line = line();
                if(peek() == "{")
                {
                    result.kind = StatementKind::Block;
                    expect("{");
                    while(peek() != "}")
                    {
                        if(position_ == tokens_.size())
                            fail("E_AST_EXPECTED", "expected '}' before end of input");
                        result.statements.push_back(statement());
                    }
                    expect("}");
                    return result;
                }
                if(peek() == "if")
                {
                    result.kind = StatementKind::Conditional;
                    expect("if");
                    expect("(");
                    result.condition = expression(0);
                    expect(")");
                    if(peek() != "{")
                        fail("E_AST_BRANCH", "expected a braced block after if condition");
                    result.then_branch = std::make_unique<Statement>(statement());
                    if(peek() == "else")
                    {
                        expect("else");
                        if(peek() != "{")
                            fail("E_AST_BRANCH", "expected a braced block after else");
                        result.else_branch = std::make_unique<Statement>(statement());
                    }
                    return result;
                }
                if(peek() == "while")
                {
                    result.kind = StatementKind::While;
                    expect("while");
                    expect("(");
                    result.condition = expression(0);
                    expect(")");
                    if(peek() != "{")
                        fail("E_AST_LOOP", "expected a braced block after while condition");
                    result.body = std::make_unique<Statement>(statement());
                    return result;
                }
                if(type_start(peek()))
                {
                    result.kind = StatementKind::Declaration;
                    result.type = type();
                    result.name = identifier();
                    if(peek() == "=")
                    {
                        expect("=");
                        result.value = expression(0);
                    }
                    expect(";");
                    return result;
                }
                if(position_ < tokens_.size() && tokens_[position_].is_identifier())
                {
                    auto target = primary();
                    if(peek() == "=")
                    {
                        result.kind = StatementKind::Assignment;
                        result.name = target->name;
                        result.target = std::move(target);
                        expect("=");
                        result.value = expression(0);
                    }
                    else
                    {
                        if(target->kind != ExpressionKind::Call)
                            fail("E_AST_STATEMENT", "expected an assignment or function call");
                        result.kind = StatementKind::Expression;
                        result.value = std::move(target);
                    }
                    expect(";");
                    return result;
                }
                fail("E_AST_STATEMENT", "expected a declaration, assignment, lexical block, if or while statement, got '" + peek() + "'");
            }
            static int precedence(const std::string& token)
            {
                if(token == "==" || token == "!=")
                    return 5;
                if(token == "<" || token == "<=" || token == ">" || token == ">=")
                    return 7;
                if(token == "+" || token == "-")
                    return 10;
                if(token == "*")
                    return 20;
                return -1;
            }

            std::unique_ptr<Expression> expression(int minimum)
            {
                auto left = primary();
                while(precedence(peek()) >= minimum)
                {
                    auto parent = std::make_unique<Expression>();
                    parent->kind = ExpressionKind::Binary;
                    parent->line = line();
                    parent->operation = peek();
                    const auto priority = precedence(peek());
                    ++position_;
                    parent->right = expression(priority + 1);
                    parent->left = std::move(left);
                    left = std::move(parent);
                }
                return left;
            }

            static std::string decode(const std::string& value)
            {
                std::string result;
                for(std::size_t i = 1; i + 1 < value.size(); ++i)
                {
                    auto c = value[i];
                    if(c == '\\')
                    {
                        c = value[++i];
                        if(c == 'n')
                            c = '\n';
                        else if(c == 't')
                            c = '\t';
                        else if(c == 'r')
                            c = '\r';
                        else if(c == '0')
                            c = '\0';
                    }
                    result += c;
                }
                return result;
            }

            std::unique_ptr<Expression> primary()
            {
                auto node = std::make_unique<Expression>();
                node->line = line();
                if(peek() == "(")
                {
                    expect("(");
                    if(type_start(peek()))
                    {
                        node->kind = ExpressionKind::Cast;
                        node->type = type();
                        expect(")");
                        node->left = primary();
                    }
                    else
                    {
                        node = expression(0);
                        expect(")");
                    }
                }
                else if(peek() == "-" || peek() == "+")
                {
                    node->kind = ExpressionKind::Unary;
                    node->operation = peek();
                    ++position_;
                    node->left = primary();
                    return node;
                }
                else if(position_ < tokens_.size() && tokens_[position_].is_string())
                {
                    node->kind = ExpressionKind::String;
                    node->text = decode(peek());
                    ++position_;
                }
                else if(position_ < tokens_.size() && tokens_[position_].is_character())
                {
                    node->kind = ExpressionKind::Character;
                    node->integer = static_cast<unsigned char>(decode(peek()).at(0));
                    ++position_;
                }
                else if(peek() == "true" || peek() == "false")
                {
                    node->kind = ExpressionKind::Boolean;
                    node->integer = peek() == "true";
                    ++position_;
                }
                else if(position_ < tokens_.size() && tokens_[position_].is_number())
                {
                    auto literal = peek();
                    const bool suffix = literal.back() == 'f' || literal.back() == 'F';
                    if(suffix || literal.find_first_of(".eE") != std::string::npos)
                    {
                        if(suffix)
                            literal.pop_back();
                        const auto converted = std::from_chars(literal.data(), literal.data() + literal.size(),
                                                              node->floating, std::chars_format::general);
                        if(converted.ec != std::errc{} || converted.ptr != literal.data() + literal.size() ||
                           !std::isfinite(node->floating) ||
                           (suffix && !std::isfinite(static_cast<float>(node->floating))))
                            fail("E_AST_LITERAL", "invalid or out-of-range floating literal");
                        node->kind = ExpressionKind::Floating;
                        node->type = {suffix ? TypeKind::Float : TypeKind::Double, {}};
                    }
                    else
                    {
                        const auto converted = std::from_chars(literal.data(), literal.data() + literal.size(), node->integer);
                        if(converted.ec != std::errc{} || converted.ptr != literal.data() + literal.size())
                            fail("E_AST_LITERAL", "expected decimal literal in range 0..2147483647, got '" + literal + "'");
                    }
                    ++position_;
                }
                else if(position_ < tokens_.size() && tokens_[position_].is_identifier())
                {
                    node->kind = ExpressionKind::Name;
                    node->name = identifier();
                    if(peek() == "(")
                    {
                        node->kind = ExpressionKind::Call;
                        expect("(");
                        if(peek() != ")")
                        {
                            while(true)
                            {
                                node->arguments.push_back(expression(0));
                                if(peek() != ",")
                                    break;
                                expect(",");
                            }
                        }
                        expect(")");
                    }
                }
                else
                    fail("E_AST_EXPR", "expected an expression, got '" + peek() + "'");
                // 成员节点拥有左侧对象；下标由语义检查写回，不使用源码偏移猜内存布局。
                while(peek() == ".")
                {
                    auto member = std::make_unique<Expression>();
                    member->kind = ExpressionKind::Member;
                    member->line = line();
                    expect(".");
                    member->name = identifier();
                    member->left = std::move(node);
                    node = std::move(member);
                }
                return node;
            }
        };
    }

    Program build(const std::vector<Token>& tokens, const std::string& source_file, bool includes_stdio)
    {
        output::print_stage("语法分析", "构造受限AST");
        auto program = Builder(tokens, source_file).run();
        program.includes_stdio = includes_stdio;
        std::cout << "受限AST构造成功" << std::endl;
        output::finish_stage();
        output::print_stage("语义分析", "检查类型、结构体成员、名字绑定与控制流初始化");
        check_names(program);
        std::cout << "类型、成员、调用参数与初始化检查成功" << std::endl;
        output::finish_stage();
        return program;
    }
}
