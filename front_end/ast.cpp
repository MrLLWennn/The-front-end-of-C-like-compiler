#include "my_compiler/ast.hpp"
#include "my_compiler/compilation_error.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace my_compiler::ast
{
    namespace
    {
        struct Binding
        {
            bool initialized = false;
        };
        using Scope = std::unordered_map<std::string, Binding>;
        using Scopes = std::vector<Scope>;

        // 从最近的词法块向外查找；块退出后，外层同名绑定自然恢复。
        Binding* find_binding(Scopes& scopes, const std::string& name)
        {
            for(auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
            {
                const auto found = scope->find(name);
                if(found != scope->end())
                    return &found->second;
            }
            return nullptr;
        }

        void check_expression(const Expression& expression, Scopes& scopes,
                              const std::string& file)
        {
            if(expression.kind == ExpressionKind::Name)
            {
                const auto* binding = find_binding(scopes, expression.name);
                if(!binding)
                    throw CompilationError("E_SEMA_NAME", file, expression.line, 0,
                                           "undeclared name '" + expression.name + "'");
                if(!binding->initialized)
                    throw CompilationError("E_SEMA_UNINITIALIZED", file, expression.line, 0,
                                           "read of uninitialized name '" + expression.name + "'");
            }
            if(expression.left)
                check_expression(*expression.left, scopes, file);
            if(expression.right)
                check_expression(*expression.right, scopes, file);
        }

        void check_statement(const Statement& statement, Scopes& scopes, const std::string& file)
        {
            switch(statement.kind)
            {
            case StatementKind::Declaration:
            {
                auto& current = scopes.back();
                if(!current.emplace(statement.name, Binding{}).second)
                    throw CompilationError("E_SEMA_LOCAL", file, statement.line, 0,
                                           "duplicate local name '" + statement.name + "'");
                // 声明点先引入新绑定，再检查初值。
                // 因此 int x = x; 不会意外读取外层同名变量。
                if(statement.value)
                {
                    check_expression(*statement.value, scopes, file);
                    current.at(statement.name).initialized = true;
                }
                break;
            }
            case StatementKind::Assignment:
            {
                auto* binding = find_binding(scopes, statement.name);
                if(!binding)
                    throw CompilationError("E_SEMA_NAME", file, statement.line, 0,
                                           "undeclared name '" + statement.name + "'");
                if(!statement.value)
                    throw std::logic_error("AST assignment has no value");
                // 右侧必须先有合法值，成功后才把目标记为已初始化。
                check_expression(*statement.value, scopes, file);
                binding->initialized = true;
                break;
            }
            case StatementKind::Conditional:
            {
                if(!statement.condition || !statement.then_branch)
                    throw std::logic_error("AST conditional has no condition or then branch");
                check_expression(*statement.condition, scopes, file);
                // 两支从同一进入状态出发；不能把成立分支的赋值带进另一支。
                auto then_scopes = scopes;
                auto else_scopes = scopes;
                check_statement(*statement.then_branch, then_scopes, file);
                if(statement.else_branch)
                    check_statement(*statement.else_branch, else_scopes, file);
                // 分支块内新声明已随块弹出，仅合并原来可见的同一层、同一名字。
                // 目前没有分支 return/break，两条路径都能正常到达汇合点。
                for(std::size_t i = 0; i < scopes.size(); ++i)
                {
                    for(auto& entry : scopes[i])
                    {
                        entry.second.initialized = then_scopes[i].at(entry.first).initialized &&
                                                   else_scopes[i].at(entry.first).initialized;
                    }
                }
                break;
            }
            case StatementKind::Block:
                scopes.emplace_back();
                for(const auto& child : statement.statements)
                    check_statement(child, scopes, file);
                scopes.pop_back();
                break;
            }
        }

        void dump_expression(const Expression& expression, std::ostream& out, int indent)
        {
            out << std::string(static_cast<std::size_t>(indent), ' ');
            switch(expression.kind)
            {
            case ExpressionKind::Integer:
                out << "整数 " << expression.integer;
                break;
            case ExpressionKind::Name:
                out << "名字 " << expression.name;
                break;
            case ExpressionKind::Unary:
                out << "一元运算 " << expression.operation;
                break;
            case ExpressionKind::Binary:
                out << "二元运算 " << expression.operation;
                break;
            }
            out << " : i32\n";
            if(expression.left)
                dump_expression(*expression.left, out, indent + 2);
            if(expression.right)
                dump_expression(*expression.right, out, indent + 2);
        }

        void dump_statement(const Statement& statement, std::ostream& out, int indent)
        {
            out << std::string(static_cast<std::size_t>(indent), ' ');
            switch(statement.kind)
            {
            case StatementKind::Declaration:
                out << "声明 " << statement.name << " : i32\n";
                break;
            case StatementKind::Assignment:
                out << "赋值 " << statement.name << "\n";
                break;
            case StatementKind::Conditional:
                out << "条件分支\n";
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "条件\n";
                dump_expression(*statement.condition, out, indent + 4);
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "成立分支\n";
                dump_statement(*statement.then_branch, out, indent + 4);
                if(statement.else_branch)
                {
                    out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "不成立分支\n";
                    dump_statement(*statement.else_branch, out, indent + 4);
                }
                break;
            case StatementKind::Block:
                out << "词法块\n";
                for(const auto& child : statement.statements)
                    dump_statement(child, out, indent + 2);
                break;
            }
            if(statement.value)
                dump_expression(*statement.value, out, indent + 2);
        }
    } // namespace

    // 顺序语句直接传播状态，结构化 if 对正常到达的两支状态求交；尚无循环不动点分析。
    void check_names(const Program& program)
    {
        std::unordered_set<std::string> functions;
        for(const auto& function : program.functions)
        {
            if(!functions.insert(function.name).second)
                throw CompilationError("E_SEMA_FUNCTION", program.source_file, function.line, 0,
                                       "duplicate function '" + function.name + "'");
            Scopes scopes(1);
            for(const auto& parameter : function.parameters)
            {
                if(!scopes.back().emplace(parameter.name, Binding{true}).second)
                    throw CompilationError("E_SEMA_PARAMETER", program.source_file, parameter.line, 0,
                                           "duplicate parameter '" + parameter.name + "'");
            }
            // 参数与函数最外层局部变量共用一个作用域，禁止同层重声明。
            for(const auto& statement : function.statements)
                check_statement(statement, scopes, program.source_file);
            if(!function.returned)
                throw std::logic_error("AST function has no return expression");
            check_expression(*function.returned, scopes, program.source_file);
        }
    }

    std::string dump(const Program& program)
    {
        std::ostringstream out;
        for(const auto& function : program.functions)
        {
            out << "函数 " << function.name << " -> i32\n";
            for(const auto& parameter : function.parameters)
                out << "  参数 " << parameter.name << " : i32\n";
            for(const auto& statement : function.statements)
                dump_statement(statement, out, 2);
            out << "  返回\n";
            dump_expression(*function.returned, out, 4);
        }
        return out.str();
    }

    void write_text(const std::string& file, const std::string& text)
    {
        std::ofstream out(file, std::ios::binary | std::ios::out);
        if(!out)
            throw std::runtime_error("cannot open artifact: " + file);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.close();
        if(!out)
            throw std::runtime_error("cannot write artifact: " + file);
    }
} // namespace my_compiler::ast
