//
// Created by 86198 on 2024/10/20.
//

#include "number_token.h"


bool my_compiler_token::NumberToken::is_number(const char& ch) noexcept
{
    return std::isdigit(ch);
}