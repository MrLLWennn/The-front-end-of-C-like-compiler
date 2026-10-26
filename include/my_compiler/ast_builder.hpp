#ifndef MY_COMPILER_AST_BUILDER_HPP
#define MY_COMPILER_AST_BUILDER_HPP
#include "my_compiler/ast.hpp"
#include "token/token.h"
#include <vector>
namespace my_compiler::ast
{
    // 过渡实现：原表驱动 Parser 接受后，再遍历同一份 Token 构建 AST。
    // 只支持明确限定的子集，尚未把 AST 构造合并到原 Parser。
    Program build(const std::vector<my_compiler_token::Token>& tokens,
        const std::string& source_file, bool includes_stdio = false);
}
#endif
