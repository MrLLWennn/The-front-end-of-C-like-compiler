//
// 创建者：86198；日期：2024/10/20。
//

#include "compiler.h"

void my_compiler::MyCompiler::compiling_aux()
{
    cout << endl << endl;
    cout << "源文件" << source_file << "正在编译中..." << endl << endl;
    divide_line();
    //前端工作
    front_end_.front_end_compiling();
}
