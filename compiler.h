//
// Created by 86198 on 2024/10/20.
//

#ifndef MY_COMPILER_COMPILER_H
#define MY_COMPILER_COMPILER_H


#include"front_end/front_end.h"


namespace my_compiler
{

    class MyCompiler
    {
    public:

        MyCompiler() = default;

    public:

        void compiling()
        {
            compiling_aux();
        }

    private:

        void compiling_aux();

    private:

       FrontEnd front_end_;
    };

}


#endif //MY_COMPILER_COMPILER_H
