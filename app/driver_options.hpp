#ifndef MY_COMPILER_DRIVER_OPTIONS_HPP
#define MY_COMPILER_DRIVER_OPTIONS_HPP

#include "my_compiler/compilation_paths.hpp"
#include <iosfwd>
#include <stdexcept>

namespace my_compiler::driver
{
    class UsageError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
    enum class EmitKind
    {
        Legacy,
        Ast,
        LLVM
    };
    struct Options
    {
        CompilationPaths paths;
        bool help = false;
        EmitKind emit = EmitKind::Legacy;
        int optimization_level = 0;
    };
    Options parse_options(int argc, char* argv[]);
    void validate_and_prepare(const CompilationPaths& paths);
    void print_help(std::ostream& out);
} // namespace my_compiler::driver
#endif
