#include "my_compiler/llvm_backend.hpp"
#include "my_compiler/compilation_error.hpp"
#include "my_compiler/stage_output.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace my_compiler::llvm_backend
{
    namespace
    {
        using Builder = llvm::IRBuilder<llvm::NoFolder>;
        using Scope = std::unordered_map<std::string, llvm::AllocaInst*>;
        using Scopes = std::vector<Scope>;
        class Lowering
        {
        public:
            Lowering(const ast::Program& program, llvm::Module& module, Builder& builder)
                : program_(program), module_(module), builder_(builder)
            {
            }

            void run()
            {
                for(const auto& structure : program_.structures)
                    structures_.emplace(structure.name,
                        llvm::StructType::create(builder_.getContext(), "struct." + structure.name));
                for(const auto& structure : program_.structures)
                {
                    std::vector<llvm::Type*> fields;
                    for(const auto& member : structure.members)
                        fields.push_back(type(member.type));
                    structures_.at(structure.name)->setBody(fields, false);
                }
                if(program_.includes_stdio)
                {
                    auto* signature = llvm::FunctionType::get(builder_.getInt32Ty(), {builder_.getPtrTy()}, true);
                    llvm::Function::Create(signature, llvm::Function::ExternalLinkage, "printf", &module_);
                }
                for(const auto& function : program_.functions)
                {
                    declarations_.emplace(function.name, &function);
                    if(module_.getFunction(function.name))
                        continue;
                    std::vector<llvm::Type*> parameters;
                    bool aggregate = function.return_type.kind == ast::TypeKind::Structure;
                    for(const auto& parameter : function.parameters)
                    {
                        parameters.push_back(type(parameter.type));
                        aggregate = aggregate || parameter.type.kind == ast::TypeKind::Structure;
                    }
                    auto* signature = llvm::FunctionType::get(type(function.return_type), parameters, false);
                    // 按值结构体仅在本翻译单元内互调，不把LLVM聚合签名冒充平台C ABI。
                    auto* generated = llvm::Function::Create(signature, aggregate ? llvm::Function::InternalLinkage
                        : llvm::Function::ExternalLinkage, function.name, &module_);
                    if(function.return_type.kind == ast::TypeKind::Char)
                        generated->addRetAttr(llvm::Attribute::SExt);
                    else if(function.return_type.kind == ast::TypeKind::Bool)
                        generated->addRetAttr(llvm::Attribute::ZExt);
                    for(std::size_t i = 0; i < function.parameters.size(); ++i)
                    {
                        if(function.parameters[i].type.kind == ast::TypeKind::Char)
                            generated->addParamAttr(i, llvm::Attribute::SExt);
                        else if(function.parameters[i].type.kind == ast::TypeKind::Bool)
                            generated->addParamAttr(i, llvm::Attribute::ZExt);
                    }
                }
                for(const auto& function : program_.functions)
                {
                    if(function.declaration_only)
                        continue;
                    auto* generated = module_.getFunction(function.name);
                    builder_.SetInsertPoint(llvm::BasicBlock::Create(builder_.getContext(), "entry", generated));
                    Scopes names(1);
                    std::size_t index = 0;
                    for(auto& argument : generated->args())
                    {
                        const auto& parameter = function.parameters[index++];
                        argument.setName(parameter.name);
                        auto* slot = create_slot(*generated, parameter.type, parameter.name);
                        builder_.CreateStore(&argument, slot);
                        names.back().emplace(parameter.name, slot);
                    }
                    for(const auto& node : function.statements)
                        statement(node, *generated, names);
                    if(function.return_type.kind == ast::TypeKind::Void)
                        builder_.CreateRetVoid();
                    else
                        builder_.CreateRet(convert(expression(*function.returned, names),
                            function.returned->type, function.return_type));
                }
            }

        private:
            const ast::Program& program_;
            llvm::Module& module_;
            Builder& builder_;
            std::unordered_map<std::string, llvm::StructType*> structures_;
            std::unordered_map<std::string, const ast::Function*> declarations_;

            llvm::Type* type(const ast::Type& value)
            {
                switch(value.kind)
                {
                case ast::TypeKind::Void:
                    return builder_.getVoidTy();
                case ast::TypeKind::Bool:
                    return builder_.getInt1Ty();
                case ast::TypeKind::Char:
                    return builder_.getInt8Ty();
                case ast::TypeKind::Int:
                    return builder_.getInt32Ty();
                case ast::TypeKind::Float:
                    return builder_.getFloatTy();
                case ast::TypeKind::Double:
                    return builder_.getDoubleTy();
                case ast::TypeKind::Structure:
                    return structures_.at(value.tag);
                case ast::TypeKind::Array:
                    return llvm::ArrayType::get(type(*value.element), value.extent);
                case ast::TypeKind::Pointer:
                case ast::TypeKind::String:
                    return builder_.getPtrTy();
                }
                throw std::logic_error("unknown checked AST type");
            }

            llvm::AllocaInst* create_slot(llvm::Function& function, const ast::Type& value, const std::string& name)
            {
                // 入口块统一分配；浮点、字符和结构体都按自己的类型和目标布局存储。
                llvm::IRBuilder<> allocation(&function.getEntryBlock(), function.getEntryBlock().begin());
                return allocation.CreateAlloca(type(value), nullptr, name + ".addr");
            }

            static llvm::AllocaInst* find(const Scopes& names, const std::string& name)
            {
                for(auto scope = names.rbegin(); scope != names.rend(); ++scope)
                {
                    const auto found = scope->find(name);
                    if(found != scope->end())
                        return found->second;
                }
                throw std::logic_error("unbound name in checked AST: " + name);
            }

            static bool named_place(const ast::Expression& node)
            {
                return node.kind == ast::ExpressionKind::Name || node.kind == ast::ExpressionKind::Index ||
                       node.kind == ast::ExpressionKind::Dereference ||
                       (node.kind == ast::ExpressionKind::Member && named_place(*node.left));
            }

            llvm::Value* address(const ast::Expression& node, const Scopes& names)
            {
                if(node.kind == ast::ExpressionKind::Name)
                    return find(names, node.name);
                if(node.kind == ast::ExpressionKind::Dereference)
                    return expression(*node.left, names);
                if(node.kind == ast::ExpressionKind::Index)
                {
                    auto* index = expression(*node.right, names);
                    index = builder_.CreateIntCast(index, builder_.getInt64Ty(), node.right->type.kind != ast::TypeKind::Bool);
                    return builder_.CreateGEP(type(node.type), expression(*node.left, names), index, "element.addr");
                }
                return builder_.CreateStructGEP(type(node.left->type), address(*node.left, names),
                    static_cast<unsigned>(node.member_index), node.name + ".addr");
            }

            llvm::Value* truth(llvm::Value* value)
            {
                if(value->getType()->isPointerTy())
                    return builder_.CreateIsNotNull(value, "truth.ptr");
                if(value->getType()->isFloatingPointTy())
                    return builder_.CreateFCmpUNE(value, llvm::ConstantFP::get(value->getType(), 0), "truth.fp");
                return builder_.CreateICmpNE(value, llvm::ConstantInt::get(value->getType(), 0), "truth.int");
            }

            llvm::Value* convert(llvm::Value* value, const ast::Type& from, const ast::Type& to)
            {
                const auto source = from.decayed();
                if(source == to || (source.kind == ast::TypeKind::Pointer && to.kind == ast::TypeKind::Pointer))
                    return value;
                if(to.kind == ast::TypeKind::Pointer)
                    return llvm::ConstantPointerNull::get(builder_.getPtrTy()); // 语义阶段仅允许整数常量0。

                if(to.kind == ast::TypeKind::Bool)
                    return truth(value);
                if(from.floating() && to.floating())
                    return from.kind == ast::TypeKind::Float
                        ? builder_.CreateFPExt(value, type(to), "fp.extend")
                        : builder_.CreateFPTrunc(value, type(to), "fp.narrow");
                if(from.floating())
                    return builder_.CreateFPToSI(value, type(to), "fp.to.int");
                if(to.floating())
                    return from.kind == ast::TypeKind::Bool ? builder_.CreateUIToFP(value, type(to), "bool.to.fp")
                        : builder_.CreateSIToFP(value, type(to), "int.to.fp");
                return builder_.CreateIntCast(value, type(to), from.kind != ast::TypeKind::Bool, "int.convert");
            }

            llvm::Value* expression(const ast::Expression& node, const Scopes& names)
            {
                using ast::ExpressionKind;
                if(node.type.kind == ast::TypeKind::Array)
                {
                    auto* zero = builder_.getInt32(0);
                    return builder_.CreateGEP(type(node.type), address(node, names), {zero, zero}, "array.decay");
                }
                switch(node.kind)
                {
                case ExpressionKind::String:
                    return builder_.CreateGlobalStringPtr(node.text, "str", 0, &module_);
                case ExpressionKind::Integer:
                case ExpressionKind::Character:
                case ExpressionKind::Boolean:
                    return llvm::ConstantInt::get(type(node.type), static_cast<std::uint64_t>(node.integer), true);
                case ExpressionKind::Floating:
                    return llvm::ConstantFP::get(type(node.type), node.floating);
                case ExpressionKind::Name:
                    return builder_.CreateLoad(type(node.type), address(node, names), node.name + ".value");
                case ExpressionKind::Member:
                    if(named_place(node))
                        return builder_.CreateLoad(type(node.type), address(node, names), node.name + ".value");
                    return builder_.CreateExtractValue(expression(*node.left, names),
                        {static_cast<unsigned>(node.member_index)}, node.name + ".value");
                case ExpressionKind::Index:
                case ExpressionKind::Dereference:
                    return builder_.CreateLoad(type(node.type), address(node, names), "indirect.value");
                case ExpressionKind::Address:
                    return address(*node.left, names);
                case ExpressionKind::Update:
                {
                    auto* slot = address(*node.left, names); // 左值只求值一次，避免p++等副作用被重复执行。
                    auto* old = builder_.CreateLoad(type(node.type), slot, "update.old");
                    const bool decrement = node.operation.find("--") != std::string::npos;
                    llvm::Value* next;
                    if(node.type.kind == ast::TypeKind::Pointer)
                        next = builder_.CreateGEP(type(*node.type.element), old, builder_.getInt64(decrement ? -1 : 1), "update.ptr");
                    else if(node.type.floating())
                        next = builder_.CreateFAdd(old, llvm::ConstantFP::get(type(node.type), decrement ? -1 : 1));
                    else
                        next = builder_.CreateAdd(old, llvm::ConstantInt::get(type(node.type), decrement ? -1 : 1, true));
                    builder_.CreateStore(next, slot);
                    return node.operation.rfind("post", 0) == 0 ? old : next;
                }
                case ExpressionKind::Cast:
                    return convert(expression(*node.left, names), node.left->type, node.type);
                case ExpressionKind::Unary:
                {
                    auto* value = convert(expression(*node.left, names), node.left->type, node.type);
                    if(node.operation == "+")
                        return value;
                    return node.type.floating() ? builder_.CreateFNeg(value, "neg.fp") : builder_.CreateNeg(value, "neg");
                }
                case ExpressionKind::Binary:
                {
                    if(node.operand_type.kind == ast::TypeKind::Pointer)
                    {
                        auto* left = expression(*node.left, names);
                        auto* right = expression(*node.right, names);
                        const bool left_pointer = node.left->type.decayed().kind == ast::TypeKind::Pointer;
                        const bool right_pointer = node.right->type.decayed().kind == ast::TypeKind::Pointer;
                        if((node.operation == "+" || node.operation == "-") && !(left_pointer && right_pointer))
                        {
                            auto* offset = left_pointer ? right : left;
                            const auto index_type = left_pointer ? node.right->type : node.left->type;
                            offset = builder_.CreateIntCast(offset, builder_.getInt64Ty(), index_type.kind != ast::TypeKind::Bool);
                            if(node.operation == "-") offset = builder_.CreateNeg(offset);
                            return builder_.CreateGEP(type(*node.operand_type.element), left_pointer ? left : right, offset, "pointer.offset");
                        }
                        if(node.operation == "-")
                            return builder_.CreateIntCast(builder_.CreatePtrDiff(type(*node.operand_type.element), left, right), builder_.getInt32Ty(), true, "pointer.distance");
                        left = convert(left, node.left->type, node.operand_type);
                        right = convert(right, node.right->type, node.operand_type);
                        llvm::CmpInst::Predicate predicate = llvm::CmpInst::ICMP_EQ;
                        if(node.operation == "!=") predicate = llvm::CmpInst::ICMP_NE;
                        else if(node.operation == "<") predicate = llvm::CmpInst::ICMP_ULT;
                        else if(node.operation == "<=") predicate = llvm::CmpInst::ICMP_ULE;
                        else if(node.operation == ">") predicate = llvm::CmpInst::ICMP_UGT;
                        else if(node.operation == ">=") predicate = llvm::CmpInst::ICMP_UGE;
                        return builder_.CreateZExt(builder_.CreateICmp(predicate, left, right), builder_.getInt32Ty(), "pointer.cmp");
                    }
                    auto* left = convert(expression(*node.left, names), node.left->type, node.operand_type);
                    auto* right = convert(expression(*node.right, names), node.right->type, node.operand_type);
                    const bool fp = node.operand_type.floating();
                    // 不加整数nsw/nuw或浮点fast-math假设；旧i32回绕语义保留。
                    if(node.operation == "+")
                        return fp ? builder_.CreateFAdd(left, right, "add.fp") : builder_.CreateAdd(left, right, "add");
                    if(node.operation == "-")
                        return fp ? builder_.CreateFSub(left, right, "sub.fp") : builder_.CreateSub(left, right, "sub");
                    if(node.operation == "*")
                        return fp ? builder_.CreateFMul(left, right, "mul.fp") : builder_.CreateMul(left, right, "mul");
                    llvm::Value* compared = nullptr;
                    if(node.operation == "<")
                        compared = fp ? builder_.CreateFCmpOLT(left, right) : builder_.CreateICmpSLT(left, right);
                    else if(node.operation == "<=")
                        compared = fp ? builder_.CreateFCmpOLE(left, right) : builder_.CreateICmpSLE(left, right);
                    else if(node.operation == ">")
                        compared = fp ? builder_.CreateFCmpOGT(left, right) : builder_.CreateICmpSGT(left, right);
                    else if(node.operation == ">=")
                        compared = fp ? builder_.CreateFCmpOGE(left, right) : builder_.CreateICmpSGE(left, right);
                    else if(node.operation == "==")
                        compared = fp ? builder_.CreateFCmpOEQ(left, right) : builder_.CreateICmpEQ(left, right);
                    else if(node.operation == "!=")
                        compared = fp ? builder_.CreateFCmpUNE(left, right) : builder_.CreateICmpNE(left, right);
                    else throw std::logic_error("unexpected checked binary operator");
                    return builder_.CreateZExt(compared, builder_.getInt32Ty(), "cmp.value");
                }
                case ExpressionKind::Call:
                {
                    auto* callee = module_.getFunction(node.name);
                    std::vector<llvm::Value*> arguments;
                    for(std::size_t i = 0; i < node.arguments.size(); ++i)
                    {
                        const auto& argument = *node.arguments[i];
                        auto* value = expression(argument, names);
                        if(callee->isVarArg())
                        {
                            // printf默认参数提升：float到double，char/bool到int；格式串仍是ptr。
                            if(argument.type.kind == ast::TypeKind::Float)
                                value = convert(value, argument.type, {ast::TypeKind::Double, {}});
                            else if(argument.type.kind == ast::TypeKind::Char || argument.type.kind == ast::TypeKind::Bool)
                                value = convert(value, argument.type, {ast::TypeKind::Int, {}});
                        }
                        else
                        {
                            // 取源码中相容签名的参数类型；原型与定义可能参数名不同。
                            const auto* declaration = declarations_.at(node.name);
                            value = convert(value, argument.type, declaration->parameters[i].type);
                        }
                        arguments.push_back(value);
                    }
                    auto* call = builder_.CreateCall(callee, arguments,
                        callee->getReturnType()->isVoidTy() ? "" : node.name + ".result");
                    call->setAttributes(callee->getAttributes());
                    return call;
                }
                }
                throw std::logic_error("unknown checked expression");
            }

            void initialize(const ast::Initializer& init, const ast::Type& value, llvm::Value* slot, const Scopes& names)
            {
                if(value.kind == ast::TypeKind::Array)
                {
                    // 省略的聚合元素归零；不是把未初始化的普通数组默认清零。
                    builder_.CreateStore(llvm::Constant::getNullValue(type(value)), slot);
                    if(!init.list)
                    {
                        for(std::size_t i = 0; i < init.value->text.size(); ++i)
                        {
                            auto* item = builder_.CreateGEP(type(value), slot, {builder_.getInt32(0), builder_.getInt32(i)});
                            builder_.CreateStore(builder_.getInt8(static_cast<unsigned char>(init.value->text[i])), item);
                        }
                    }
                    else
                    {
                        for(std::size_t i = 0; i < init.entries.size(); ++i)
                        {
                            auto* item = builder_.CreateGEP(type(value), slot, {builder_.getInt32(0), builder_.getInt32(i)});
                            initialize(init.entries[i], *value.element, item, names);
                        }
                    }
                    return;
                }
                if(init.list) initialize(init.entries.at(0), value, slot, names);
                else builder_.CreateStore(convert(expression(*init.value, names), init.value->type, value), slot);
            }

            void statement(const ast::Statement& node, llvm::Function& function, Scopes& names)
            {
                using ast::StatementKind;
                switch(node.kind)
                {
                case StatementKind::Expression:
                    expression(*node.value, names);
                    break;
                case StatementKind::Declaration:
                {
                    auto* slot = create_slot(function, node.type, node.name);
                    names.back().emplace(node.name, slot);
                    if(node.initializer) initialize(*node.initializer, node.type, slot, names);
                    break;
                }
                case StatementKind::Assignment:
                {
                    auto* slot = address(*node.target, names);
                    auto* right = expression(*node.value, names);
                    if(node.operation == "=")
                        right = convert(right, node.value->type, node.target->type);
                    else
                    {
                        llvm::Value* old = builder_.CreateLoad(type(node.target->type), slot);
                        if(node.type.kind == ast::TypeKind::Pointer)
                        {
                            right = builder_.CreateIntCast(right, builder_.getInt64Ty(), node.value->type.kind != ast::TypeKind::Bool);
                            if(node.operation == "-=") right = builder_.CreateNeg(right);
                            right = builder_.CreateGEP(type(*node.type.element), old, right);
                        }
                        else
                        {
                            old = convert(old, node.target->type, node.type);
                            right = convert(right, node.value->type, node.type);
                            if(node.operation == "+=") right = node.type.floating() ? builder_.CreateFAdd(old, right) : builder_.CreateAdd(old, right);
                            else if(node.operation == "-=") right = node.type.floating() ? builder_.CreateFSub(old, right) : builder_.CreateSub(old, right);
                            else right = node.type.floating() ? builder_.CreateFMul(old, right) : builder_.CreateMul(old, right);
                            right = convert(right, node.type, node.target->type);
                        }
                    }
                    builder_.CreateStore(right, slot);
                    break;
                }
                case StatementKind::Conditional:
                {
                    auto* condition = truth(expression(*node.condition, names));
                    auto& context = builder_.getContext();
                    auto* yes = llvm::BasicBlock::Create(context, "if.then", &function);
                    auto* no = node.else_branch ? llvm::BasicBlock::Create(context, "if.else", &function) : nullptr;
                    auto* end = llvm::BasicBlock::Create(context, "if.end", &function);
                    builder_.CreateCondBr(condition, yes, no ? no : end);
                    builder_.SetInsertPoint(yes);
                    statement(*node.then_branch, function, names);
                    builder_.CreateBr(end);
                    if(node.else_branch)
                    {
                        builder_.SetInsertPoint(no);
                        statement(*node.else_branch, function, names);
                        builder_.CreateBr(end);
                    }
                    builder_.SetInsertPoint(end);
                    break;
                }
                case StatementKind::While:
                {
                    auto& context = builder_.getContext();
                    auto* condition = llvm::BasicBlock::Create(context, "while.condition", &function);
                    auto* body = llvm::BasicBlock::Create(context, "while.body", &function);
                    auto* end = llvm::BasicBlock::Create(context, "while.end", &function);
                    builder_.CreateBr(condition);
                    builder_.SetInsertPoint(condition);
                    builder_.CreateCondBr(truth(expression(*node.condition, names)), body, end);
                    builder_.SetInsertPoint(body);
                    statement(*node.body, function, names);
                    builder_.CreateBr(condition);
                    builder_.SetInsertPoint(end);
                    break;
                }
                case StatementKind::For:
                {
                    names.emplace_back();
                    if(node.setup) statement(*node.setup, function, names);
                    auto& context = builder_.getContext();
                    auto* condition = llvm::BasicBlock::Create(context, "for.condition", &function);
                    auto* body = llvm::BasicBlock::Create(context, "for.body", &function);
                    auto* step = llvm::BasicBlock::Create(context, "for.step", &function);
                    auto* end = llvm::BasicBlock::Create(context, "for.end", &function);
                    builder_.CreateBr(condition);
                    builder_.SetInsertPoint(condition);
                    auto* check = node.condition ? truth(expression(*node.condition, names)) : builder_.getTrue();
                    builder_.CreateCondBr(check, body, end);
                    builder_.SetInsertPoint(body);
                    statement(*node.body, function, names);
                    builder_.CreateBr(step);
                    builder_.SetInsertPoint(step);
                    if(node.step) statement(*node.step, function, names);
                    builder_.CreateBr(condition);
                    builder_.SetInsertPoint(end);
                    names.pop_back();
                    break;
                }
                case StatementKind::Block:
                    names.emplace_back();
                    for(const auto& child : node.statements)
                        statement(child, function, names);
                    names.pop_back();
                    break;
                }
            }
        };
        // 校验 IR 的结构和类型一致性；不能替代源语言名字检查或运行结果验证。
        void verify(llvm::Module& module, const std::string& file)
        {
            std::string message;
            llvm::raw_string_ostream errors(message);
            const bool invalid = llvm::verifyModule(module, &errors);
            errors.flush();
            if(invalid)
                throw CompilationError("E_LLVM_VERIFY", file, 0, 0, message);
        }
        std::string module_text(llvm::Module& module)
        {
            std::string text;
            llvm::raw_string_ostream out(text);
            module.print(out, nullptr);
            out.flush();
            return text;
        }
        // O0 不运行优化管线；O2 注册分析与代理后运行 LLVM 标准模块优化管线。
        void optimize(llvm::Module& module, llvm::TargetMachine* target, int level)
        {
            if(level == 0)
                return;
            if(level != 2)
                throw std::invalid_argument("LLVM subset supports optimization levels 0 and 2");
            // 分析管理器互相持有代理，按此声明顺序保证析构时依赖仍有效。
            llvm::LoopAnalysisManager loops;
            llvm::FunctionAnalysisManager functions;
            llvm::CGSCCAnalysisManager calls;
            llvm::ModuleAnalysisManager modules;
            llvm::PassBuilder passes(target);
            // 注册的是各层分析的构造方式；具体结果由分析管理器按需计算并缓存。
            passes.registerModuleAnalyses(modules);
            passes.registerCGSCCAnalyses(calls);
            passes.registerFunctionAnalyses(functions);
            passes.registerLoopAnalyses(loops);
            // 代理连接不同 IR 层级；LLVM 内置 Pass 声明哪些分析在变换后仍然有效。
            passes.crossRegisterProxies(loops, functions, calls, modules);
            // O2 是 LLVM 提供的完整编排，不是本项目自行实现的一次变换。
            auto pipeline = passes.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
            pipeline.run(module, modules);
        }
    } // namespace
    // 建立目标布局与模块，将 AST 降为 IR，校验/优化后写出 IR 和原生 .o。

    void emit(const ast::Program& program, const CompilationPaths& paths, int optimization_level)
    {
        output::print_stage("中间代码生成", "构造并校验LLVM IR");
        if(llvm::InitializeNativeTarget() || llvm::InitializeNativeTargetAsmPrinter())
            throw std::runtime_error("cannot initialize the native LLVM backend");
        const auto triple = llvm::sys::getDefaultTargetTriple();
        std::string target_error;
        const auto* target = llvm::TargetRegistry::lookupTarget(triple, target_error);
        if(!target)
            throw std::runtime_error("LLVM target lookup: " + target_error);
        llvm::TargetOptions target_options;
        const auto codegen_level = optimization_level == 0
            ? llvm::CodeGenOptLevel::None : llvm::CodeGenOptLevel::Default;
        std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(
            triple, "generic", "", target_options, llvm::Reloc::PIC_, std::nullopt, codegen_level));
        if(!machine)
            throw std::runtime_error("cannot create LLVM target machine");
        llvm::LLVMContext context;
        llvm::Module module("my_compiler", context);
        module.setSourceFileName(program.source_file);
        module.setTargetTriple(triple);
        module.setDataLayout(machine->createDataLayout());
        Builder builder(context); // NoFolder 保留原始指令，让构建 IR 与优化变换分开可观察。
        Lowering(program, module, builder).run();
        verify(module, program.source_file);
        const auto before = module_text(module);
        std::cout << "目标平台：" << triple << std::endl;
        std::cout << "原始LLVM IR构造及校验成功" << std::endl;
        output::finish_stage();
        output::print_stage("优化", optimization_level == 0
            ? "保留原始LLVM IR（O0）" : "运行LLVM标准优化管线（O2）");
        optimize(module, machine.get(), optimization_level);
        verify(module, program.source_file);
        const auto after = module_text(module);
        std::cout << "优化阶段及IR校验成功" << std::endl;
        output::finish_stage();
        // 语法、名字及 IR 校验通过后才写出本轮 AST、IR、目标文件。
        // 多个文件不构成原子事务；写入中途失败仍可能有部分产物，应使用新输出目录。
        output::print_stage("中间结果输出", "保存AST与优化前后IR");
        ast::write_text(paths.ast_file, ast::dump(program));
        std::cout << "文件写入" << paths.ast_file << std::endl;
        ast::write_text(paths.llvm_before_file, before);
        std::cout << "文件写入" << paths.llvm_before_file << std::endl;
        ast::write_text(paths.llvm_after_file, after);
        std::cout << "文件写入" << paths.llvm_after_file << std::endl;
        output::finish_stage();
        output::print_stage("目标代码生成", "生成原生目标文件");
        std::error_code error;
        llvm::raw_fd_ostream object(paths.object_file, error, llvm::sys::fs::OF_None);
        if(error)
            throw std::runtime_error("cannot open LLVM object file: " + error.message());
        llvm::legacy::PassManager emission;
        if(machine->addPassesToEmitFile(emission, object, nullptr, llvm::CodeGenFileType::ObjectFile))
            throw std::runtime_error("LLVM target cannot emit an object file");
        emission.run(module);
        object.close();
        if(object.has_error())
        {
            const auto message = object.error().message();
            object.clear_error(); // 清除已处理的流错误，避免流析构再次以致命错误终止进程。
            throw std::runtime_error("cannot write LLVM object file: " + message);
        }
        std::cout << "文件写入" << paths.object_file << std::endl;
        std::cout << "原生目标文件生成成功" << std::endl;
        output::finish_stage();
    }
} // namespace my_compiler::llvm_backend
