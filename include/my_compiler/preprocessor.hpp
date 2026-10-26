#ifndef MY_COMPILER_PREPROCESSOR_HPP
#define MY_COMPILER_PREPROCESSOR_HPP
#include <array>
#include <string>
#include <vector>

namespace my_compiler::preprocessing
{
    struct Result
    {
        bool includes_stdio = false;
        std::vector<std::string> headers;
    };

    // 本地头文件真实展开；stdio.h导入受限标准库契约，不展开平台内部声明。
    // 完成所有依赖读取和输出冲突检查后，才写出去注释结果。
    Result prepare(const std::string& source, const std::string& output,
                   const std::array<std::string, 17>& generated_files);
}
#endif
