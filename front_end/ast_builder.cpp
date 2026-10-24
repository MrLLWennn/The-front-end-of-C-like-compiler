#include "my_compiler/ast_builder.hpp"
#include "my_compiler/compilation_error.hpp"
#include "my_compiler/stage_output.hpp"
#include <algorithm>
#include <charconv>
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
                Program program;
                program.source_file = file_;
                while(position_ < tokens_.size())
                    program.functions.push_back(function());
                if(program.functions.empty())
                    fail("E_AST_EMPTY", "expected a function definition");
                return program;
            }

        private:
            const std::vector<Token>& tokens_;
            std::string file_;
            std::size_t position_ = 0;

            std::string peek() const
            {
                return position_ < tokens_.size() ? tokens_[position_].get_attribute_value() : "<eof>";
            }

            std::size_t line() const
            {
                return position_ < tokens_.size() ? tokens_[position_].get_line() : 0;
            }

            [[noreturn]] void fail(const std::string& code, const std::string& message) const
            {
                throw CompilationError(code, file_, line(), 0, message);
            }

            void expect(const std::string& value)
            {
                if(peek() != value)
                    fail("E_AST_EXPECTED", "expected '" + value + "', got '" + peek() + "'");
                ++position_;
            }

            std::string identifier()
            {
                if(position_ == tokens_.size() || !tokens_[position_].is_identifier())
                    fail("E_AST_NAME", "expected an identifier, got '" + peek() + "'");
                return tokens_[position_++].get_attribute_value();
            }

            Function function()
            {
                Function result;
                result.line = line();
                if(peek() != "int")
                    fail("E_AST_TYPE", "LLVM subset requires an int return type");
                expect("int");
                result.name = identifier();
                expect("(");
                if(peek() == "void")
                {
                    expect("void");
                }
                else if(peek() != ")")
                {
                    while(true)
                    {
                        if(peek() != "int")
                            fail("E_AST_TYPE", "LLVM subset requires int parameters");
                        expect("int");
                        const auto parameter_line = line();
                        result.parameters.push_back(Parameter{identifier(), parameter_line});
                        if(peek() != ",")
                            break;
                        expect(",");
                    }
                }
                expect(")");
                expect("{");
                // 仍保留唯一的函数顶层末尾 return；此前允许声明、赋值、词法块和条件分支。
                while(peek() != "return")
                {
                    if(peek() == "}" || position_ == tokens_.size())
                        fail("E_AST_RETURN", "expected a final return expression");
                    result.statements.push_back(statement());
                }
                expect("return");
                result.returned = expression(0);
                expect(";");
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
                    // 本轮要求两支都使用花括号，避免隐式作用域及悬空 else 的歧义。
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
                if(peek() == "int")
                {
                    result.kind = StatementKind::Declaration;
                    expect("int");
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
                    result.kind = StatementKind::Assignment;
                    result.name = identifier();
                    expect("=");
                    result.value = expression(0);
                    expect(";");
                    return result;
                }
                fail("E_AST_STATEMENT", "expected an int declaration, assignment, lexical block or if statement, got '" + peek() + "'");
            }

            static int precedence(const std::string& token)
            {
                if(token == "+" || token == "-")
                    return 10;
                if(token == "*")
                    return 20;
                return -1;
            }

            // 优先级爬升：先读操作数，再按门槛把运算符与右侧表达式合成父节点。
            std::unique_ptr<Expression> expression(int minimum_precedence)
            {
                auto left = primary();
                while(precedence(peek()) >= minimum_precedence)
                {
                    const auto operation = peek();
                    const auto operation_line = line();
                    const int priority = precedence(operation);
                    ++position_;
                    auto right = expression(priority + 1); // 右侧提高门槛，使同优先级运算按左结合组织
                    auto node = std::make_unique<Expression>();
                    node->kind = ExpressionKind::Binary;
                    node->line = operation_line;
                    node->operation = operation[0];
                    // 将两棵子树交给父节点：移动后局部 left/right 为空，节点地址不变。
                    node->left = std::move(left);
                    node->right = std::move(right);
                    // 新父节点成为下一轮的左子树根；node 交出所有权后为空。
                    left = std::move(node);
                }
                return left;
            }

            std::unique_ptr<Expression> primary()
            {
                if(peek() == "(")
                {
                    expect("(");
                    auto node = expression(0);
                    expect(")");
                    return node;
                }
                auto node = std::make_unique<Expression>();
                node->line = line();
                if(peek() == "-")
                {
                    expect("-");
                    node->kind = ExpressionKind::Unary;
                    node->operation = '-';
                    node->left = primary();
                    return node;
                }
                if(position_ < tokens_.size() && tokens_[position_].is_number())
                {
                    const auto literal = peek();
                    const auto converted = std::from_chars(literal.data(), literal.data() + literal.size(), node->integer);
                    if(converted.ec != std::errc{} || converted.ptr != literal.data() + literal.size())
                        fail("E_AST_LITERAL", "expected decimal literal in range 0..2147483647, got '" + literal + "'");
                    ++position_;
                    return node;
                }
                if(position_ < tokens_.size() && tokens_[position_].is_identifier())
                {
                    node->kind = ExpressionKind::Name;
                    node->name = identifier();
                    return node;
                }
                fail("E_AST_EXPR", "expected an integer expression, got '" + peek() + "'");
            }
        };
    } // namespace

    Program build(const std::vector<Token>& tokens, const std::string& source_file)
    {
        output::print_stage("语法分析", "构造受限AST");
        auto program = Builder(tokens, source_file).run();
        std::cout << "受限AST构造成功" << std::endl;
        output::finish_stage();

        output::print_stage("语义分析", "检查名字绑定、词法作用域与分支初始化");
        check_names(program);
        std::cout << "名字绑定、作用域与分支初始化检查成功" << std::endl;
        output::finish_stage();
        return program;
    }
} // namespace my_compiler::ast
