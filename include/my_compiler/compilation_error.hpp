#ifndef MY_COMPILER_COMPILATION_ERROR_HPP
#define MY_COMPILER_COMPILATION_ERROR_HPP

#include <cstddef>
#include <stdexcept>
#include <string>

namespace my_compiler
{

    // 源程序处理失败时抛出，由命令行入口捕获。
    // 位置对应标明的中间文件，尚未映射回原始源码。
    class CompilationError : public std::runtime_error
    {
    public:
        CompilationError(const std::string& code, const std::string& file,
            std::size_t line, std::size_t column,
            const std::string& message)
            : std::runtime_error(format(code, file, line, column, message))
        {
        }

    private:
        static std::string format(const std::string& code, const std::string& file,
            std::size_t line, std::size_t column,
            const std::string& message)
        {
            std::string result = file;
            if(line != 0)
            {
                result += ":" + std::to_string(line);
                if(column != 0)
                    result += ":" + std::to_string(column);
            }
            return result + ": error[" + code + "]: " + message;
        }
    };

} // namespace my_compiler
#endif
