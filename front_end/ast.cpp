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
        case TypeKind::Pointer:
            return type_name(*type.element) + "*";
        case TypeKind::Array:
            return "[" + std::to_string(type.extent) + " x " + type_name(*type.element) + "]";
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
            // 地址逃逸后不冒充完整别名分析：间接写入及数组内容由运行时有效性约束负责。
            bool escaped = false;
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
                if(type.kind == TypeKind::Pointer)
                {
                    auto target = type.element;
                    while(target->kind == TypeKind::Pointer || target->kind == TypeKind::Array) target = target->element;
                    if(target->kind == TypeKind::Structure && !structures_.count(target->tag))
                        fail("E_SEMA_STRUCT", line, "unknown struct pointer target '" + target->tag + "'");
                    return;
                }
                if(type.kind == TypeKind::Array)
                {
                    if(type.extent == 0 || type.extent > 65536)
                        fail("E_SEMA_ARRAY", line, "invalid array extent");
                    object_type(*type.element, line, before);
                    std::size_t count = 1;
                    const Type* part = &type;
                    while(part->kind == TypeKind::Array)
                    {
                        if(count > 1048576 / part->extent)
                            fail("E_SEMA_ARRAY", line, "array exceeds 1048576 elements");
                        count *= part->extent;
                        part = part->element.get();
                    }
                    return;
                }
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
                if(type.kind == TypeKind::Array)
                    return; // 不将未知下标读写伪装成“整个数组已初始化”。
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
                if(node.kind == ExpressionKind::Dereference || node.kind == ExpressionKind::Index)
                {
                    const auto pointer = expression(*node.left, scopes).decayed();
                    if(pointer.kind != TypeKind::Pointer || pointer.element->kind == TypeKind::Void)
                        fail("E_SEMA_POINTER", node.line, "dereference/index requires a pointer to an object");
                    if(node.right && !expression(*node.right, scopes).integral())
                        fail("E_SEMA_POINTER", node.line, "array index must have integral type");
                    node.type = *pointer.element;
                    return {nullptr, node.type, {}, "indirect object"};
                }
                fail("E_SEMA_LVALUE", node.line, "assignment requires a modifiable object lvalue");
            }

            static bool named_place(const Expression& node)
            {
                return node.kind == ExpressionKind::Name || node.kind == ExpressionKind::Index ||
                       node.kind == ExpressionKind::Dereference ||
                       (node.kind == ExpressionKind::Member && named_place(*node.left));
            }

            void require_initialized(const Place& target, std::size_t line) const
            {
                if(!target.binding || target.binding->escaped) return;
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
                if(!target.binding) return;
                for(const auto& leaf : leaves(target.type, target.path))
                    target.binding->initialized.insert(leaf);
            }

            static bool compatible_pointer(const Type& from, const Type& to)
            {
                return from.kind == TypeKind::Pointer && to.kind == TypeKind::Pointer &&
                    (*from.element == *to.element || from.element->kind == TypeKind::Void || to.element->kind == TypeKind::Void);
            }

            void conversion(Type from, const Type& to, std::size_t line, bool null = false) const
            {
                from = from.decayed();
                if((from.numeric() && to.numeric()) || (from == to && from.kind == TypeKind::Structure) ||
                   compatible_pointer(from, to) || (null && to.kind == TypeKind::Pointer) ||
                   (from.kind == TypeKind::Pointer && to.kind == TypeKind::Bool))
                    return;
                fail("E_SEMA_TYPE", line, "cannot convert " + type_name(from) + " to " + type_name(to));
            }

            void converted(Expression& value, const Type& to, Scopes& scopes)
            {
                const auto from = expression(value, scopes);
                conversion(from, to, value.line, value.kind == ExpressionKind::Integer && value.integer == 0);
            }

            void check_initializer(Initializer& init, const Type& to, Scopes& scopes, std::size_t line)
            {
                if(to.kind == TypeKind::Array)
                {
                    if(!init.list)
                    {
                        if(to.element->kind == TypeKind::Char && init.value->kind == ExpressionKind::String &&
                           init.value->text.size() <= to.extent)
                        {
                            expression(*init.value, scopes);
                            return;
                        }
                        fail("E_SEMA_INITIALIZER", line, "array requires a brace list or char string initializer");
                    }
                    if(init.entries.size() > to.extent)
                        fail("E_SEMA_INITIALIZER", line, "too many array initializer elements");
                    for(auto& item : init.entries) check_initializer(item, *to.element, scopes, line);
                    return;
                }
                if(init.list)
                {
                    if(init.entries.size() != 1)
                        fail("E_SEMA_INITIALIZER", line, "scalar initializer requires one element");
                    check_initializer(init.entries[0], to, scopes, line);
                    return;
                }
                converted(*init.value, to, scopes);
            }

            void scalar(const Type& type, std::size_t line) const
            {
                if(!type.numeric() && type.decayed().kind != TypeKind::Pointer)
                    fail("E_SEMA_TYPE", line, "condition requires a numeric value or pointer");
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
                    return node.type = Type::pointer({TypeKind::Char, {}});
                case ExpressionKind::Name:
                case ExpressionKind::Index:
                case ExpressionKind::Dereference:
                case ExpressionKind::Member:
                    if(named_place(node))
                    {
                        const auto target = place(node, scopes);
                        require_initialized(target, node.line);
                        return node.type;
                    }
                    node.type = member_type(expression(*node.left, scopes), node);
                    if(node.type.kind == TypeKind::Array)
                        fail("E_SEMA_LVALUE", node.line, "array member of a temporary struct is not supported");
                    return node.type;
                case ExpressionKind::Address:
                {
                    const auto target = place(*node.left, scopes);
                    if(target.binding) target.binding->escaped = true;
                    return node.type = Type::pointer(target.type);
                }
                case ExpressionKind::Update:
                {
                    const auto target = place(*node.left, scopes);
                    require_initialized(target, node.line);
                    if(target.type.kind == TypeKind::Array || target.type.kind == TypeKind::Bool ||
                       (!target.type.numeric() && target.type.kind != TypeKind::Pointer) ||
                       (target.type.kind == TypeKind::Pointer && target.type.element->kind == TypeKind::Void))
                        fail("E_SEMA_POINTER", node.line, "increment/decrement requires numeric or object pointer lvalue");
                    initialize(target);
                    return node.type = target.type;
                }
                case ExpressionKind::Unary:
                {
                    const auto operand = expression(*node.left, scopes);
                    if(!operand.numeric()) fail("E_SEMA_TYPE", node.line, "unary arithmetic requires a numeric value");
                    return node.type = arithmetic_type(operand, operand);
                }
                case ExpressionKind::Cast:
                {
                    const auto operand = expression(*node.left, scopes);
                    if(!(operand.numeric() && node.type.numeric()) &&
                       !(operand.decayed().kind == TypeKind::Pointer && node.type.kind == TypeKind::Pointer) &&
                       !(node.left->kind == ExpressionKind::Integer && node.left->integer == 0 && node.type.kind == TypeKind::Pointer))
                        fail("E_SEMA_TYPE", node.line, "cast requires numeric types or object pointers");
                    return node.type;
                }
                case ExpressionKind::Binary:
                {
                    const auto left = expression(*node.left, scopes).decayed();
                    const auto right = expression(*node.right, scopes).decayed();
                    if(left.kind == TypeKind::Pointer || right.kind == TypeKind::Pointer)
                    {
                        const auto pointer = left.kind == TypeKind::Pointer ? left : right;
                        const auto other = left.kind == TypeKind::Pointer ? right : left;
                        const bool offset = (node.operation == "+" || (node.operation == "-" && left.kind == TypeKind::Pointer)) && other.integral();
                        if(offset && pointer.element->kind != TypeKind::Void)
                            return node.type = node.operand_type = pointer;
                        if(node.operation == "-" && left == right && pointer.element->kind != TypeKind::Void)
                        {
                            node.operand_type = pointer;
                            return node.type = {TypeKind::Int, {}};
                        }
                        const bool equality = node.operation == "==" || node.operation == "!=";
                        const auto& integer = left.kind == TypeKind::Pointer ? *node.right : *node.left;
                        const bool null = integer.kind == ExpressionKind::Integer && integer.integer == 0;
                        if((equality && (compatible_pointer(left, right) || null)) ||
                           (!equality && left == right && (node.operation == "<" || node.operation == "<=" || node.operation == ">" || node.operation == ">=")))
                        {
                            node.operand_type = pointer;
                            return node.type = {TypeKind::Int, {}};
                        }
                        fail("E_SEMA_POINTER", node.line, "invalid pointer arithmetic or comparison");
                    }
                    if(!left.numeric() || !right.numeric())
                        fail("E_SEMA_TYPE", node.line, "arithmetic requires a numeric value");
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
                                const auto value = expression(argument, scopes).decayed();
                                if(value.kind != TypeKind::Pointer || value.element->kind != TypeKind::Char)
                                    fail("E_SEMA_FORMAT", argument.line, "%s requires a string literal or char pointer");
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
                            converted(*node.arguments[i], signature.parameters[i], scopes);
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
                    if(!scopes.back().emplace(node.name, Binding{node.type, false, {}}).second)
                        fail("E_SEMA_LOCAL", node.line, "duplicate local name '" + node.name + "'");
                    if(node.initializer)
                    {
                        check_initializer(*node.initializer, node.type, scopes, node.line);
                        initialize({&scopes.back().at(node.name), node.type, {}, node.name});
                    }
                    break;
                }
                case StatementKind::Assignment:
                {
                    const auto target = place(*node.target, scopes);
                    if(target.type.kind == TypeKind::Array)
                        fail("E_SEMA_LVALUE", node.line, "arrays are not assignable");
                    if(node.operation != "=")
                    {
                        require_initialized(target, node.line);
                        const auto right = expression(*node.value, scopes);
                        if(target.type.kind == TypeKind::Pointer)
                        {
                            if((node.operation != "+=" && node.operation != "-=") || !right.integral() || target.type.element->kind == TypeKind::Void)
                                fail("E_SEMA_POINTER", node.line, "invalid compound pointer assignment");
                            node.type = target.type;
                        }
                        else
                        {
                            if(!target.type.numeric() || !right.numeric()) fail("E_SEMA_TYPE", node.line, "compound assignment requires numeric values");
                            node.type = arithmetic_type(target.type, right);
                        }
                    }
                    else converted(*node.value, target.type, scopes);
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
                            entry.second.escaped = yes[i].at(entry.first).escaped || no[i].at(entry.first).escaped;
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
                    for(std::size_t i = 0; i < scopes.size(); ++i)
                        for(auto& entry : scopes[i]) entry.second.escaped = entry.second.escaped || body[i].at(entry.first).escaped;
                    break;
                }
                case StatementKind::For:
                {
                    scopes.emplace_back();
                    if(node.setup) statement(*node.setup, scopes);
                    if(node.condition) scalar(expression(*node.condition, scopes), node.line);
                    auto body = scopes;
                    statement(*node.body, body);
                    if(node.step) statement(*node.step, body);
                    for(std::size_t i = 0; i < scopes.size(); ++i)
                        for(auto& entry : scopes[i]) entry.second.escaped = entry.second.escaped || body[i].at(entry.first).escaped;
                    scopes.pop_back();
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
                    auto inserted_parameter = scopes.back().emplace(parameter.name, Binding{parameter.type, false, {}});
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
                    converted(*function.returned, function.return_type, scopes);
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
            case ExpressionKind::Address:
                out << "取地址"; break;
            case ExpressionKind::Dereference:
                out << "解引用"; break;
            case ExpressionKind::Index:
                out << "数组下标"; break;
            case ExpressionKind::Update:
                out << "自增减 " << node.operation; break;
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
            case StatementKind::For:
                out << "for循环（初始化、条件、循环体、步进）\n";
                if(node.setup) dump_statement(*node.setup, out, indent + 2);
                if(node.condition) dump_expression(*node.condition, out, indent + 2);
                dump_statement(*node.body, out, indent + 2);
                if(node.step) dump_statement(*node.step, out, indent + 2);
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
            if(node.initializer)
            {
                const auto visit = [&](const auto& self, const Initializer& init, int level) -> void
                {
                    if(init.value) dump_expression(*init.value, out, level);
                    for(const auto& item : init.entries) self(self, item, level + 2);
                };
                visit(visit, *node.initializer, indent + 2);
            }
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
