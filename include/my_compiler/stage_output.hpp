#ifndef MY_COMPILER_STAGE_OUTPUT_HPP
#define MY_COMPILER_STAGE_OUTPUT_HPP

#include <iostream>

namespace my_compiler::output
{
    // 沿用旧前端的过程/步骤标题，统一新增阶段的正常输出。
    inline void print_stage(const char* phase, const char* step)
    {
        std::cout << "************------------ 过程：" << phase
                  << "----步骤：" << step << "------------************" << std::endl;
    }

    inline void finish_stage()
    {
        std::cout << "------------------------------------------------------------------------------" << std::endl;
    }
}

#endif
