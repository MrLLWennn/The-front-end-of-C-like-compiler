//
// 创建者：86198；日期：2024/12/2。
//

#ifndef MY_COMPILER_NAME_GENERATOR_H
#define MY_COMPILER_NAME_GENERATOR_H

#include"../header_files.h"

//此头文件用于中间代码 对变量名在编译器内部生成唯一的名字


class NameGenerator
{
private:
    //
    std::unordered_map<string, unsigned> name_tags_;
public:
    NameGenerator() = default;

    //选择的版本
    static constexpr unsigned version = 0xffffffff;

    //
    string gen_name(const string& tag, unsigned ex = version)
    {
        auto iter = name_tags_.find(tag);
        unsigned count = 0;
        if(iter != name_tags_.end())
        {
            count = iter->second + 1;
        }
        if(ex != version)
        {
            count = ex;
        }
        std::stringstream ss;
        ss << tag << "." << count;
        if(ex == version)
        {
            name_tags_[tag] = count;
        }
        return ss.str();
    }

    NameGenerator* name_tags()
    {
        return const_cast<NameGenerator*>(this);
    }

};







#endif //MY_COMPILER_NAME_GENERATOR_H
