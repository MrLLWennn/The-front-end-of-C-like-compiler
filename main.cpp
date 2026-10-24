#include "compiler.h"
#include "app/driver_options.hpp"
#include "my_compiler/stage_output.hpp"
#ifdef MY_COMPILER_WITH_LLVM
#include "my_compiler/llvm_backend.hpp"
#endif
#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        const auto options = my_compiler::driver::parse_options(argc, argv);
        if(options.help)
        {
            my_compiler::driver::print_help(std::cout);
            return 0;
        }
#ifndef MY_COMPILER_WITH_LLVM
        if(options.emit == my_compiler::driver::EmitKind::LLVM)
            throw my_compiler::driver::UsageError("LLVM backend is disabled; configure MY_COMPILER_ENABLE_LLVM=ON");
#endif
        my_compiler::driver::validate_and_prepare(options.paths);
        my_compiler::configure_compilation_paths(options.paths);
        my_compiler::MyCompiler compiler;
        if(options.emit == my_compiler::driver::EmitKind::Legacy)
        {
            compiler.compiling();
        }
        else
        {
            auto program = compiler.build_typed_program();
            if(options.emit == my_compiler::driver::EmitKind::Ast)
            {
                my_compiler::output::print_stage("中间结果输出", "保存受限AST");
                my_compiler::ast::write_text(options.paths.ast_file, my_compiler::ast::dump(program));
                std::cout << "文件写入" << options.paths.ast_file << std::endl;
                my_compiler::output::finish_stage();
            }
            else
            {
#ifdef MY_COMPILER_WITH_LLVM
                my_compiler::llvm_backend::emit(program, options.paths, options.optimization_level);
#endif
            }
        }
        return 0;
    }
    catch(const my_compiler::driver::UsageError& error)
    {
        std::cerr << "my_compiler: " << error.what() << '\n';
        return 2;
    }
    catch(const std::exception& error)
    {
        std::cerr << "my_compiler: " << error.what() << '\n';
        return 1;
    }
}
