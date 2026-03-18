//
// Created by 86198 on 2024/10/20.
//

#ifndef MY_COMPILER_NUMBER_TOKEN_H
#define MY_COMPILER_NUMBER_TOKEN_H


#include"../token.h"


namespace my_compiler_token
{


    class NumberToken : public Token
    {
    public:
        [[nodiscard("no use return")]] static bool is_number(const char& ch) noexcept;

        NumberToken(const string& value, const size_t& line)
        :Token(TokenType::NUMBER, value, line)
        {
        }
    };

}


#endif //MY_COMPILER_NUMBER_TOKEN_H
