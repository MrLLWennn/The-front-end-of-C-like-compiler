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

        llvm::AllocaInst* find_slot(const Scopes& scopes, const std::string& name)
        {
            for(auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
            {
                const auto found = scope->find(name);
                if(found != scope->end())
                    return found->second;
            }
            throw std::logic_error("unbound name in checked AST: " + name);
        }

        // 临时 Builder 只负责入口块分配，不改变主 Builder 当前语句的插入点。
        // 作用域结束撤掉名字绑定；入口块槽位可继续由 LLVM 按生命周期优化。
        llvm::AllocaInst* create_slot(llvm::Function& function, Builder& builder,
                                     const std::string& name)
        {
            llvm::IRBuilder<> allocation(&function.getEntryBlock(), function.getEntryBlock().begin());
            return allocation.CreateAlloca(builder.getInt32Ty(), nullptr, name + ".addr");
        }

        llvm::Value* expression(const ast::Expression& node, Builder& builder,
                                const Scopes& names)
        {
            switch(node.kind)
            {
            case ast::ExpressionKind::Integer:
                return llvm::ConstantInt::get(builder.getInt32Ty(), static_cast<std::uint64_t>(node.integer), true);
            case ast::ExpressionKind::Name:
                // 名字现在映射到槽位地址；表达式需要的是槽位当前存放的值。
                return builder.CreateLoad(builder.getInt32Ty(), find_slot(names, node.name), node.name + ".value");
            case ast::ExpressionKind::Unary:
                return builder.CreateNeg(expression(*node.left, builder, names), "neg");
            case ast::ExpressionKind::Binary:
            {
                auto* left = expression(*node.left, builder, names);
                auto* right = expression(*node.right, builder, names);
                // 当前语言定义 i32 加减乘按位回绕，不添加 nsw/nuw 溢出假设。
                switch(node.operation)
                {
                case '+':
                    return builder.CreateAdd(left, right, "add");
                case '-':
                    return builder.CreateSub(left, right, "sub");
                case '*':
                    return builder.CreateMul(left, right, "mul");
                default:
                    throw std::logic_error("unexpected binary operator in checked AST");
                }
            }
            }
            throw std::logic_error("unexpected expression kind in checked AST");
        }

        void statement(const ast::Statement& node, llvm::Function& function,
                       Builder& builder, Scopes& names)
        {
            switch(node.kind)
            {
            case ast::StatementKind::Declaration:
            {
                auto* slot = create_slot(function, builder, node.name);
                // 与前端一致：声明点先引入新绑定，初值随后求值。
                names.back().emplace(node.name, slot);
                if(node.value)
                    builder.CreateStore(expression(*node.value, builder, names), slot);
                break;
            }
            case ast::StatementKind::Assignment:
                builder.CreateStore(expression(*node.value, builder, names), find_slot(names, node.name));
                break;
            case ast::StatementKind::Conditional:
            {
                auto* value = expression(*node.condition, builder, names);
                auto* condition = builder.CreateICmpNE(value, builder.getInt32(0), "if.condition");
                auto& context = builder.getContext();
                auto* then_block = llvm::BasicBlock::Create(context, "if.then", &function);
                auto* else_block = node.else_branch
                    ? llvm::BasicBlock::Create(context, "if.else", &function) : nullptr;
                auto* merge_block = llvm::BasicBlock::Create(context, "if.end", &function);
                builder.CreateCondBr(condition, then_block, else_block ? else_block : merge_block);

                builder.SetInsertPoint(then_block);
                statement(*node.then_branch, function, builder, names);
                // 递归生成嵌套 if 后，当前块可能已是内层汇合块。
                // 继续从 Builder 当前块连到外层汇合点，不回到最初的 then 块追加。
                builder.CreateBr(merge_block);

                if(node.else_branch)
                {
                    builder.SetInsertPoint(else_block);
                    statement(*node.else_branch, function, builder, names);
                    builder.CreateBr(merge_block);
                }
                builder.SetInsertPoint(merge_block);
                break;
            }
            case ast::StatementKind::Block:
                names.emplace_back();
                for(const auto& child : node.statements)
                    statement(child, function, builder, names);
                names.pop_back();
                break;
            }
        }

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
        for(const auto& function : program.functions)
        {
            std::vector<llvm::Type*> parameter_types(function.parameters.size(), builder.getInt32Ty());
            auto* signature = llvm::FunctionType::get(builder.getInt32Ty(), parameter_types, false);
            auto* generated = llvm::Function::Create(signature, llvm::Function::ExternalLinkage,
                                                      function.name, &module);
            auto* entry = llvm::BasicBlock::Create(context, "entry", generated);
            builder.SetInsertPoint(entry);
            Scopes names(1);
            std::size_t index = 0;
            for(auto& argument : generated->args())
            {
                const auto& parameter = function.parameters[index++];
                argument.setName(parameter.name);
                auto* slot = create_slot(*generated, builder, parameter.name);
                builder.CreateStore(&argument, slot);
                names.back().emplace(parameter.name, slot);
            }
            for(const auto& node : function.statements)
                statement(node, *generated, builder, names);
            builder.CreateRet(expression(*function.returned, builder, names));
        }
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
