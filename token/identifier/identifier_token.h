//
// 创建者：86198；日期：2024/10/20。
//

#ifndef MY_COMPILER_IDENTIFIER_TOKEN_H
#define MY_COMPILER_IDENTIFIER_TOKEN_H


#include"../token.h"


namespace my_compiler_token
{


    class IdentifierToken : public Token
    {
    public:

        IdentifierToken(const string& value, const size_t& line)
        :Token(TokenType::IDENTIFIER, value, line)
        {
        }
    };


}


#endif //MY_COMPILER_IDENTIFIER_TOKEN_H
