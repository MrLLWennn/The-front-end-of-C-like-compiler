//
// 创建者：86198；日期：2024/10/20。
//

#include "operator_token.h"





set<char> my_compiler_token::OperatorToken::operators_ = {
        ADDITION,
        SUBTRACTION,
        MULTIPLICATION,
        DIVISION,
        LESS_THAN,
        GREATER_THAN,
        ASSIGNMENT,
        POWER,
        NEGATION,
        '&',
};


bool my_compiler_token::OperatorToken::is_operator(const char& ch) noexcept
{
    return operators_.find(ch) != operators_.end();
}


bool my_compiler_token::OperatorToken::is_addition_token(const Token &token) noexcept
{
    Token addition_token = OperatorToken(ADDITION);
    return token == addition_token;
}


bool my_compiler_token::OperatorToken::is_subtraction_token(const Token &token) noexcept
{
    Token subtraction_token = OperatorToken(SUBTRACTION);
    return token == subtraction_token;
}


bool my_compiler_token::OperatorToken::is_multiplication_token(const Token &token) noexcept {
    Token multiplication_token = OperatorToken(MULTIPLICATION);
    return token == multiplication_token;
}


bool my_compiler_token::OperatorToken::is_division_token(const Token &token) noexcept
{
    Token division_token = OperatorToken(DIVISION);
    return token == division_token;
}


bool my_compiler_token::OperatorToken::is_less_than_token(const Token &token) noexcept
{
    Token less_than_token = OperatorToken(LESS_THAN);
    return token == less_than_token;
}


bool my_compiler_token::OperatorToken::is_greater_than_token(const Token &token) noexcept
{
    Token greater_than_token = OperatorToken(GREATER_THAN);
    return token == greater_than_token;
}


bool my_compiler_token::OperatorToken::is_assignment_token(const Token &token) noexcept
{
    Token assignment_token = OperatorToken(ASSIGNMENT);
    return token == assignment_token;
}


bool my_compiler_token::OperatorToken::is_power_token(const my_compiler_token::Token &token) noexcept
{
    Token power_token = OperatorToken(POWER);
    return token == power_token;
}

bool my_compiler_token::OperatorToken::is_negation_token(const my_compiler_token::Token &token) noexcept
{
    Token negation_token = OperatorToken(NEGATION);
    return token == negation_token;
}


