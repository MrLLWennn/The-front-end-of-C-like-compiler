//
// Created by 86198 on 2024/10/30.
//

#ifndef MY_COMPILER_ERRORTABLE_H
#define MY_COMPILER_ERRORTABLE_H


#include"ErrorDefines.h"
#include"../header_files.h"


namespace my_compiler_error_handler
{


    class ErrorTable
    {
    private:
        //错误信息表
        map<ErrorDefines, string> error_info_;
        //错误出现的行数
        size_t line_;
        //错误数量
        size_t total_numbers_;

    public:

        ErrorTable() : line_(0), total_numbers_(0)
        {
            //加入 expected 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(EXPECTED, "expected "));
            //加入 missing 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(MISSING, "missing "));
            //加入 redundant 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDUNDANT, "redundant "));
            //加入 redeclared 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDECLARED_IDENTIFIER, "redeclared identifier "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDECLARED_PROCESS, "redeclared process "));
            //加入 undeclared 的
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDECLARED_IDENTIFIER, "undeclared identifier "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDECLARED_PROCESS, "undeclared process "));
            //加入 undefined 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDEFINED_PROCESS, "undefined process "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDEFINED_PROCESS, "undefined process "));
            //加入 illegal 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_DEFINE, "illegal define "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_WORD, "illegal word "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_RVALUE_ASSIGN, "illegal rvalue assign"));
            //加入 incompatible 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(INCOMPATIBLE_VAR_LIST, "incompatible variable list: "));
            //加入 no matching function 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(NO_MATCHING_FUNCTION, "no matching function call to "));
            //加入 默认 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(EXPECTED_UNQUALIFIED_ID, "expected unqualified-id "));
        }

        explicit ErrorTable(const size_t& line)
                : line_(line), total_numbers_(0)
        {
            //加入 expected 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(EXPECTED, "expected "));
            //加入 missing 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(MISSING, "missing "));
            //加入 redundant 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDUNDANT, "redundant "));
            //加入 redeclared 的错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDECLARED_IDENTIFIER, "redeclared identifier "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(REDECLARED_PROCESS, "redeclared process "));
            //加入 undeclared 的
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDECLARED_IDENTIFIER, "undeclared identifier "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDECLARED_PROCESS, "undeclared process "));
            //加入 undefined 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDEFINED_PROCESS, "undefined process "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(UNDEFINED_PROCESS, "undefined process "));
            //加入 illegal 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_DEFINE, "illegal define "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_WORD, "illegal word "));
            error_info_.insert(std::make_pair<ErrorDefines, string>(ILLEGAL_RVALUE_ASSIGN, "illegal rvalue assign"));
            //加入 incompatible 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(INCOMPATIBLE_VAR_LIST, "incompatible variable list: "));
            //加入 no matching function 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(NO_MATCHING_FUNCTION, "no matching function call to "));
            //加入 默认 错误
            error_info_.insert(std::make_pair<ErrorDefines, string>(EXPECTED_UNQUALIFIED_ID, "expected unqualified-id "));
        }


        string find(ErrorDefines error)
        {
            if(error_info_.find(error) != error_info_.end())
                total_numbers_++;
            return error_info_.find(error)->second;
        }


        [[nodiscard]] size_t total_error_number() const noexcept
        {
            return total_numbers_;
        }


    };


}




#endif //MY_COMPILER_ERRORTABLE_H
