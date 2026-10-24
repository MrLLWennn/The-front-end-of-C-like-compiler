#ifndef MY_COMPILER_LLVM_BACKEND_HPP
#define MY_COMPILER_LLVM_BACKEND_HPP
#include "my_compiler/ast.hpp"
#include "my_compiler/compilation_paths.hpp"
namespace my_compiler::llvm_backend
{
    // 接口只暴露本项目类型，前端和驱动无需包含 LLVM 类型。
    void emit(const ast::Program& program, const CompilationPaths& paths, int optimization_level);
}
#endif
