//
// Created by 86198 on 2024/10/21.
//



#include"token.h"


string my_compiler_token::get_token_name(my_compiler_token::TokenType type)
{
    switch(type)
    {
        case TokenType::DELIMITER:
            return "Delimiter";
        case TokenType::IDENTIFIER:
            return "Identifier";
        case TokenType::END:
            return "EndOfFile";
        case TokenType::NUMBER:
            return "Number";
        case TokenType::KEYWORD:
            return "Keyword";
        case TokenType::OPERATOR:
            return "Operator";
        case TokenType::ERR:
            return "Error";
        case TokenType::BOOLEAN:
            return "Boolean";
        default:
            return "Default";
    }
}



