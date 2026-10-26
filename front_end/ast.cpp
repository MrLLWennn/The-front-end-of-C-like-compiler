#include "my_compiler/ast.hpp"
#include "my_compiler/compilation_error.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace my_compiler::ast
{

    std::string type_name(const Type& type)
    {
        switch(type.kind)
        {
        case TypeKind::Void:
            return "void";
        case TypeKind::Bool:
            return "i1";
        case TypeKind::Char:
            return "i8";
        case TypeKind::Int:
            return "i32";
        case TypeKind::Float:
            return "float";
        case TypeKind::Double:
            return "double";
        case TypeKind::Structure:
            return "struct " + type.tag;
        case TypeKind::String:
            return "字符串常量";
        }
        throw std::logic_error("unknown AST type");
    }

    Type arithmetic_type(Type left, Type right)
    {
        if(left.kind == TypeKind::Double || right.kind == TypeKind::Double)
            return {TypeKind::Double, {}};
        if(left.kind == TypeKind::Float || right.kind == TypeKind::Float)
            return {TypeKind::Float, {}};
        return {TypeKind::Int, {}}; // bool/char先做整数提升。
    }
    namespace
    {
        struct Binding
        {
            Type type;
            // 标量使用空路径；结构体按叶成员路径记录，不能把p.x赋值算作整个p已初始化。
            std::unordered_set<std::string> initialized;
        };
        using Scope = std::unordered_map<std::string, Binding>;
        using Scopes = std::vector<Scope>;
        struct Signature
        {
            Type returned;
            std::vector<Type> parameters;
            bool defined = false;
            bool printf_contract = false;
        };
        class Checker
        {
        public:
            explicit Checker(Program& program) : program_(program)
            {
            }

            void run()
            {
                // 只允许按值引用已经完整定义的结构体，禁止直接/间接递归对象布局。
                for(const auto& structure : program_.structures)
                {
                    if(!structures_.emplace(structure.name, &structure).second)
                        fail("E_SEMA_STRUCT", structure.line, "duplicate struct '" + structure.name + "'");
                    if(structure.members.empty() || structure.members.size() > 64)
                        fail("E_SEMA_STRUCT", structure.line, "struct requires 1..64 members");
                    std::unordered_set<std::string> names;
                    for(const auto& member : structure.members)
                    {
                        if(!names.insert(member.name).second)
                            fail("E_SEMA_MEMBER", member.line, "duplicate member '" + member.name + "'");
                        object_type(member.type, member.line, structure.source_order);
                    }
                    leaves({TypeKind::Structure, structure.name});
                }
                if(program_.includes_stdio)
                    functions_.emplace("printf", Signature{{TypeKind::Int, {}}, {}, false, true});
                for(auto& function : program_.functions)
                    check_function(function);
            }

        private:
            Program& program_;
            std::unordered_map<std::string, const Structure*> structures_;
            std::unordered_map<std::string, Signature> functions_;
            std::size_t current_order_ = 0;

            [[noreturn]] void fail(const std::string& code, std::size_t line, const std::string& text) const
            {
                throw CompilationError(code, program_.source_file, line, 0, text);
            }

            void object_type(const Type& type, std::size_t line, std::size_t before) const
            {
                if(type.kind == TypeKind::Void || type.kind == TypeKind::String)
                    fail("E_SEMA_TYPE", line, "object or parameter cannot have void/string type");
                if(type.kind == TypeKind::Structure)
                {
                    const auto found = structures_.find(type.tag);
                    if(found == structures_.end() || found->second->source_order >= before)
                        fail("E_SEMA_STRUCT", line, "struct '" + type.tag + "' must be fully defined before use");
                }
            }

            void collect_leaves(const Type& type, const std::string& prefix,
                                std::vector<std::string>& result, unsigned depth) const
            {
                if(depth > 16 || result.size() >= 1024)
                    fail("E_SEMA_STRUCT", 0, "struct layout exceeds depth/leaf limit");
                if(type.kind != TypeKind::Structure)
                    result.push_back(prefix);
                else
                {
                    for(const auto& member : structures_.at(type.tag)->members)
                        collect_leaves(member.type, prefix.empty() ? member.name : prefix + "." + member.name,
                                       result, depth + 1);
                }
            }

            std::vector<std::string> leaves(const Type& type, const std::string& prefix = {}) const
            {
                std::vector<std::string> result;
                collect_leaves(type, prefix, result, 0);
                return result;
            }

            static Binding* find(Scopes& scopes, const std::string& name)
            {
                for(auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
                {
                    const auto found = scope->find(name);
                    if(found != scope->end())
                        return &found->second;
                }
                return nullptr;
            }

            Type member_type(const Type& owner, Expression& node) const
            {
                if(owner.kind != TypeKind::Structure)
                    fail("E_SEMA_MEMBER", node.line, "member access requires a struct object");
                const auto& members = structures_.at(owner.tag)->members;
                for(std::size_t i = 0; i < members.size(); ++i)
                {
                    if(members[i].name == node.name)
                    {
                        node.member_index = i;
                        return members[i].type;
                    }
                }
                fail("E_SEMA_MEMBER", node.line, "struct '" + owner.tag + "' has no member '" + node.name + "'");
            }
            struct Place
            {
                Binding* binding;
                Type type;
                std::string path;
                std::string root;
            };

            Place place(Expression& node, Scopes& scopes)
            {
                if(node.kind == ExpressionKind::Name)
                {
                    auto* binding = find(scopes, node.name);
                    if(!binding)
                        fail("E_SEMA_NAME", node.line, "undeclared name '" + node.name + "'");
                    node.type = binding->type;
                    return {binding, node.type, {}, node.name};
                }
                if(node.kind == ExpressionKind::Member)
                {
                    auto base = place(*node.left, scopes);
                    node.type = member_type(base.type, node);
                    base.type = node.type;
                    base.path = base.path.empty() ? node.name : base.path + "." + node.name;
                    return base;
                }
                fail("E_SEMA_LVALUE", node.line, "assignment requires a named object or member");
            }

            static bool named_place(const Expression& node)
            {
                return node.kind == ExpressionKind::Name ||
                       (node.kind == ExpressionKind::Member && named_place(*node.left));
            }

            void require_initialized(const Place& target, std::size_t line) const
            {
                for(const auto& leaf : leaves(target.type, target.path))
                {
                    if(!target.binding->initialized.count(leaf))
                        fail("E_SEMA_UNINITIALIZED", line, target.path.empty() && target.type.kind != TypeKind::Structure
                            ? "read of uninitialized name '" + target.root + "'"
                            : "read of uninitialized member '" + target.root + "." + leaf + "'");
                }
            }

            void initialize(const Place& target) const
            {
                for(const auto& leaf : leaves(target.type, target.path))
                    target.binding->initialized.insert(leaf);
            }

            void conversion(const Type& from, const Type& to, std::size_t line) const
            {
                if((from.numeric() && to.numeric()) || (from == to && from.kind == TypeKind::Structure))
                    return;
                fail("E_SEMA_TYPE", line, "cannot convert " + type_name(from) + " to " + type_name(to));
            }

            void scalar(const Type& type, std::size_t line) const
            {
                if(!type.numeric())
                    fail("E_SEMA_TYPE", line, "condition/arithmetic requires a numeric value");
            }

            std::vector<char> formats(const Expression& node) const
            {
                const auto& text = node.text;
                std::vector<char> result;
                for(std::size_t i = 0; i < text.size(); ++i)
                {
                    if(text[i] != '%')
                        continue;
                    if(++i == text.size())
                        fail("E_SEMA_FORMAT", node.line, "incomplete printf conversion");
                    if(text[i] == '%')
                        continue;
                    std::string flags;
                    while(i < text.size() && std::string("-+ 0").find(text[i]) != std::string::npos)
                        flags += text[i++];
                    unsigned width = 0;
                    while(i < text.size() && text[i] >= '0' && text[i] <= '9')
                    {
                        width = width * 10 + static_cast<unsigned>(text[i++] - '0');
                        if(width > 1024)
                            fail("E_SEMA_FORMAT", node.line, "printf width exceeds supported limit 1024");
                    }
                    bool precision = false;
                    if(i < text.size() && text[i] == '.')
                    {
                        precision = true;
                        ++i;
                        unsigned digits = 0;
                        const auto start = i;
                        while(i < text.size() && text[i] >= '0' && text[i] <= '9')
                        {
                            digits = digits * 10 + static_cast<unsigned>(text[i++] - '0');
                            if(digits > 128)
                                fail("E_SEMA_FORMAT", node.line, "printf precision exceeds 128");
                        }
                        if(i == start)
                            fail("E_SEMA_FORMAT", node.line, "printf precision requires fixed digits");
                    }
                    if(i == text.size() || std::string("dicsfFeEgG").find(text[i]) == std::string::npos)
                        fail("E_SEMA_FORMAT", node.line, "supported printf conversions are %d, %i, %c, %s, %f, %e, %g and %%");
                    if((text[i] == 'c' || text[i] == 's') &&
                       (flags.find_first_not_of('-') != std::string::npos || precision))
                        fail("E_SEMA_FORMAT", node.line, "only width and '-' flag are supported for %c and %s");
                    result.push_back(text[i]);
                }
                return result;
            }

            Type expression(Expression& node, Scopes& scopes)
            {
                switch(node.kind)
                {
                case ExpressionKind::Integer:
                case ExpressionKind::Character:
                    return node.type = {TypeKind::Int, {}}; // C字符常量的类型是int。
                case ExpressionKind::Boolean:
                    return node.type = {TypeKind::Bool, {}};
                case ExpressionKind::Floating:
                    return node.type;
                case ExpressionKind::String:
                    fail("E_SEMA_TYPE", node.line, "string literals are only supported as printf arguments");
                case ExpressionKind::Name:
                case ExpressionKind::Member:
                    if(named_place(node))
                    {
                        const auto target = place(node, scopes);
                        require_initialized(target, node.line);
                        return node.type;
                    }
                    node.type = member_type(expression(*node.left, scopes), node);
                    return node.type;
                case ExpressionKind::Unary:
                {
                    const auto operand = expression(*node.left, scopes);
                    scalar(operand, node.line);
                    return node.type = arithmetic_type(operand, operand);
                }
                case ExpressionKind::Cast:
                {
                    const auto operand = expression(*node.left, scopes);
                    if(!operand.numeric() || !node.type.numeric())
                        fail("E_SEMA_TYPE", node.line, "casts currently require numeric source and destination");
                    return node.type;
                }
                case ExpressionKind::Binary:
                {
                    const auto left = expression(*node.left, scopes);
                    const auto right = expression(*node.right, scopes);
                    scalar(left, node.line);
                    scalar(right, node.line);
                    node.operand_type = arithmetic_type(left, right);
                    const bool arithmetic = node.operation == "+" || node.operation == "-" || node.operation == "*";
                    return node.type = arithmetic ? node.operand_type : Type{TypeKind::Int, {}};
                }
                case ExpressionKind::Call:
                {
                    if(find(scopes, node.name))
                        fail("E_SEMA_CALL", node.line, "called name '" + node.name + "' is a local object");
                    const auto found = functions_.find(node.name);
                    if(found == functions_.end())
                        fail("E_SEMA_CALL", node.line, "call to undefined or not yet declared function '" + node.name + "'");
                    const auto& signature = found->second;
                    if(signature.printf_contract)
                    {
                        if(node.arguments.empty() || node.arguments[0]->kind != ExpressionKind::String)
                            fail("E_SEMA_FORMAT", node.line, "printf requires a string literal format");
                        node.arguments[0]->type = {TypeKind::String, {}};
                        const auto conversions = formats(*node.arguments[0]);
                        if(node.arguments.size() != conversions.size() + 1)
                            fail("E_SEMA_FORMAT", node.line, "printf format argument count mismatch");
                        for(std::size_t i = 0; i < conversions.size(); ++i)
                        {
                            auto& argument = *node.arguments[i + 1];
                            const auto spec = conversions[i];
                            const bool string = argument.kind == ExpressionKind::String;
                            if(spec == 's')
                            {
                                if(!string)
                                    fail("E_SEMA_FORMAT", argument.line, "%s requires a string literal");
                                argument.type = {TypeKind::String, {}};
                                continue;
                            }
                            if(string)
                                fail("E_SEMA_FORMAT", argument.line, "integer conversion requires an int argument; floating conversion requires a float or double argument");
                            const auto type = expression(argument, scopes);
                            const bool fp = std::string("fFeEgG").find(spec) != std::string::npos;
                            if(fp ? !type.floating() : (!type.numeric() || type.floating()))
                                fail("E_SEMA_FORMAT", argument.line, fp ? "floating conversion requires a float or double argument" : "integer conversion requires an int argument");
                        }
                    }
                    else
                    {
                        if(node.arguments.size() != signature.parameters.size())
                            fail("E_SEMA_ARGUMENTS", node.line, "wrong argument count for '" + node.name + "': expected " +
                                 std::to_string(signature.parameters.size()) + ", got " + std::to_string(node.arguments.size()));
                        for(std::size_t i = 0; i < node.arguments.size(); ++i)
                            conversion(expression(*node.arguments[i], scopes), signature.parameters[i], node.arguments[i]->line);
                    }
                    return node.type = signature.returned;
                }
                }
                throw std::logic_error("unknown AST expression");
            }

            void statement(Statement& node, Scopes& scopes)
            {
                switch(node.kind)
                {
                case StatementKind::Expression:
                    expression(*node.value, scopes);
                    break;
                case StatementKind::Declaration:
                {
                    object_type(node.type, node.line, current_order_);
                    if(!scopes.back().emplace(node.name, Binding{node.type, {}}).second)
                        fail("E_SEMA_LOCAL", node.line, "duplicate local name '" + node.name + "'");
                    if(node.value)
                    {
                        conversion(expression(*node.value, scopes), node.type, node.line);
                        initialize({&scopes.back().at(node.name), node.type, {}, node.name});
                    }
                    break;
                }
                case StatementKind::Assignment:
                {
                    const auto target = place(*node.target, scopes);
                    conversion(expression(*node.value, scopes), target.type, node.line);
                    initialize(target);
                    break;
                }
                case StatementKind::Conditional:
                {
                    scalar(expression(*node.condition, scopes), node.line);
                    auto yes = scopes, no = scopes;
                    statement(*node.then_branch, yes);
                    if(node.else_branch)
                        statement(*node.else_branch, no);
                    // 对每一叶成员求交；一支写p.x、另一支写p.y不等于p已完整初始化。
                    for(std::size_t i = 0; i < scopes.size(); ++i)
                    {
                        for(auto& entry : scopes[i])
                        {
                            auto& initialized = entry.second.initialized;
                            initialized.clear();
                            for(const auto& path : yes[i].at(entry.first).initialized)
                                if(no[i].at(entry.first).initialized.count(path))
                                    initialized.insert(path);
                        }
                    }
                    break;
                }
                case StatementKind::While:
                {
                    scalar(expression(*node.condition, scopes), node.line);
                    auto body = scopes;
                    statement(*node.body, body);
                    break;
                }
                case StatementKind::Block:
                    scopes.emplace_back();
                    for(auto& child : node.statements)
                        statement(child, scopes);
                    scopes.pop_back();
                    break;
                }
            }

            void check_function(Function& function)
            {
                current_order_ = function.source_order;
                if(function.return_type.kind != TypeKind::Void)
                    object_type(function.return_type, function.line, current_order_);
                Signature candidate;
                candidate.returned = function.return_type;
                bool aggregate = function.return_type.kind == TypeKind::Structure;
                for(const auto& parameter : function.parameters)
                {
                    object_type(parameter.type, parameter.line, current_order_);
                    candidate.parameters.push_back(parameter.type);
                    aggregate = aggregate || parameter.type.kind == TypeKind::Structure;
                }
                if(function.name == "main" &&
                   (function.return_type.kind != TypeKind::Int || !function.parameters.empty()))
                    fail("E_SEMA_MAIN", function.line, "main requires int return and no parameters in this subset");
                if(aggregate && std::none_of(program_.functions.begin(), program_.functions.end(),
                    [&](const Function& other) { return other.name == function.name && !other.declaration_only; }))
                    fail("E_SEMA_ABI", function.line, "struct-by-value functions require a definition in this translation unit");
                auto inserted = functions_.emplace(function.name, candidate);
                auto& signature = inserted.first->second;
                if(signature.printf_contract || signature.returned != candidate.returned || signature.parameters != candidate.parameters)
                    fail("E_SEMA_FUNCTION", function.line, "incompatible declaration for function '" + function.name + "'");
                if(!function.declaration_only)
                {
                    if(signature.defined)
                        fail("E_SEMA_FUNCTION", function.line, "duplicate function '" + function.name + "'");
                    signature.defined = true;
                }
                Scopes scopes(1);
                for(const auto& parameter : function.parameters)
                {
                    auto inserted_parameter = scopes.back().emplace(parameter.name, Binding{parameter.type, {}});
                    if(!inserted_parameter.second)
                        fail("E_SEMA_PARAMETER", parameter.line, "duplicate parameter '" + parameter.name + "'");
                    initialize({&inserted_parameter.first->second, parameter.type, {}, parameter.name});
                }
                if(function.declaration_only)
                    return;
                for(auto& node : function.statements)
                    statement(node, scopes);
                if(function.return_type.kind == TypeKind::Void)
                {
                    if(function.returned)
                        fail("E_SEMA_RETURN", function.line, "void function cannot return a value");
                }
                else
                {
                    if(!function.returned)
                        fail("E_SEMA_RETURN", function.line, "non-void function requires a return value");
                    conversion(expression(*function.returned, scopes), function.return_type, function.line);
                }
            }
        };
        void dump_expression(const Expression& node, std::ostream& out, int indent)
        {
            out << std::string(static_cast<std::size_t>(indent), ' ');
            switch(node.kind)
            {
            case ExpressionKind::Integer:
                out << "整数 " << node.integer; break;
            case ExpressionKind::Character:
                out << "字符码 " << node.integer; break;
            case ExpressionKind::Boolean:
                out << "布尔 " << node.integer; break;
            case ExpressionKind::Floating:
                out << "浮点 " << node.floating; break;
            case ExpressionKind::Name:
                out << "名字 " << node.name; break;
            case ExpressionKind::Member:
                out << "成员 " << node.name << "（索引=" << node.member_index << "）"; break;
            case ExpressionKind::Unary:
                out << "一元运算 " << node.operation; break;
            case ExpressionKind::Binary:
                out << "二元运算 " << node.operation; break;
            case ExpressionKind::Call:
                out << "函数调用 " << node.name; break;
            case ExpressionKind::Cast:
                out << "显式数值转换"; break;
            case ExpressionKind::String:
                out << "字符串 \"";
                for(const char c : node.text)
                {
                    if(c == '\n')
                        out << "\\n";
                    else if(c == '\t')
                        out << "\\t";
                    else if(c == '\r')
                        out << "\\r";
                    else if(c == '\\' || c == '"')
                        out << '\\' << c;
                    else out << c;
                }
                out << '"';
                break;
            }
            out << " : " << type_name(node.type) << '\n';
            if(node.left)
                dump_expression(*node.left, out, indent + 2);
            if(node.right)
                dump_expression(*node.right, out, indent + 2);
            for(const auto& argument : node.arguments)
                dump_expression(*argument, out, indent + 2);
        }
        std::string target_name(const Expression& node)
        {
            return node.kind == ExpressionKind::Member ? target_name(*node.left) + "." + node.name : node.name;
        }
        void dump_statement(const Statement& node, std::ostream& out, int indent)
        {
            out << std::string(static_cast<std::size_t>(indent), ' ');
            switch(node.kind)
            {
            case StatementKind::Expression:
                out << "调用语句\n"; break;
            case StatementKind::Declaration:
                out << "声明 " << node.name << " : " << type_name(node.type) << '\n'; break;
            case StatementKind::Assignment:
                out << "赋值 " << target_name(*node.target) << '\n'; break;
            case StatementKind::Block:
                out << "词法块\n";
                for(const auto& child : node.statements)
                    dump_statement(child, out, indent + 2);
                break;
            case StatementKind::Conditional:
                out << "条件分支\n";
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "条件\n";
                dump_expression(*node.condition, out, indent + 4);
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "成立分支\n";
                dump_statement(*node.then_branch, out, indent + 4);
                if(node.else_branch)
                {
                    out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "不成立分支\n";
                    dump_statement(*node.else_branch, out, indent + 4);
                }
                break;
            case StatementKind::While:
                out << "while循环\n";
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "条件\n";
                dump_expression(*node.condition, out, indent + 4);
                out << std::string(static_cast<std::size_t>(indent + 2), ' ') << "循环体\n";
                dump_statement(*node.body, out, indent + 4);
                break;
            }
            if(node.value)
                dump_expression(*node.value, out, indent + 2);
        }
    }

    void check_names(Program& program)
    {
        Checker(program).run();
    }

    std::string dump(const Program& program)
    {
        std::ostringstream out;
        for(const auto& structure : program.structures)
        {
            out << "结构体 " << structure.name << '\n';
            for(const auto& member : structure.members)
                out << "  成员 " << member.name << " : " << type_name(member.type) << '\n';
        }
        for(const auto& function : program.functions)
        {
            if(function.declaration_only)
            {
                out << "函数声明 " << function.name << " -> " << type_name(function.return_type)
                    << "（参数数=" << function.parameters.size() << "）\n";
                continue;
            }
            out << "函数 " << function.name << " -> " << type_name(function.return_type) << '\n';
            for(const auto& parameter : function.parameters)
                out << "  参数 " << parameter.name << " : " << type_name(parameter.type) << '\n';
            for(const auto& node : function.statements)
                dump_statement(node, out, 2);
            out << "  返回\n";
            if(function.returned)
                dump_expression(*function.returned, out, 4);
            else out << "    void\n";
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
}
