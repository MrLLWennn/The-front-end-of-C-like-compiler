//
// Created by 86198 on 2024/10/20.
//

#include "delimiter_token.h"


bool my_compiler_token::DelimiterToken::is_delimiter(const char &ch)
{
    return delimiters_.find(ch) != delimiters_.end();
}

bool my_compiler_token::DelimiterToken::is_dot_token(const my_compiler_token::Token& token)
{
    Token dot_token = DelimiterToken(DOT);
    return token == dot_token;
}

bool my_compiler_token::DelimiterToken::is_left_parentheses_token(const my_compiler_token::Token& token)
{
    Token left_parentheses_token = DelimiterToken(LEFT_PARENTHESES);
    return token == left_parentheses_token;
}

bool my_compiler_token::DelimiterToken::is_right_parentheses_token(const my_compiler_token::Token& token)
{
    Token right_parentheses_token = DelimiterToken(RIGHT_PARENTHESES);
    return token == right_parentheses_token;
}

bool my_compiler_token::DelimiterToken::is_comma_token(const my_compiler_token::Token& token)
{
    Token comma_token = DelimiterToken(COMMA);
    return token == comma_token;
}

bool my_compiler_token::DelimiterToken::is_left_curly_brace(const my_compiler_token::Token& token)
{
    Token left_curly_brace_token = DelimiterToken(LEFT_CURLY_BRACE);
    return token == left_curly_brace_token;
}

bool my_compiler_token::DelimiterToken::is_right_curly_brace(const my_compiler_token::Token& token)
{
    Token right_curly_brace_token = DelimiterToken(RIGHT_CURLY_BRACE);
    return token == right_curly_brace_token;
}

bool my_compiler_token::DelimiterToken::is_semicolon_token(const my_compiler_token::Token& token)
{
    Token semicolon_token = DelimiterToken(SEMI_COLON);
    return token == semicolon_token;
}

bool my_compiler_token::DelimiterToken::is_left_square_bracket(const my_compiler_token::Token& token)
{
    Token left_square_bracket_token = DelimiterToken(LEFT_SQUARE_BRACKET);
    return token == left_square_bracket_token;
}

bool my_compiler_token::DelimiterToken::is_right_square_bracket(const my_compiler_token::Token& token)
{
    Token right_square_bracket_token = DelimiterToken(RIGHT_SQUARE_BRACKET);
    return token == right_square_bracket_token;
}



set<char> my_compiler_token::DelimiterToken::delimiters_ = {
        DOT,
        LEFT_PARENTHESES,
        RIGHT_PARENTHESES,
        COMMA,
        LEFT_CURLY_BRACE,
        RIGHT_CURLY_BRACE,
        SEMI_COLON,
        LEFT_SQUARE_BRACKET,
        RIGHT_SQUARE_BRACKET,
        COLON
};

bool my_compiler_token::DelimiterToken::is_colon(const my_compiler_token::Token &token)
{
    Token colon_token = DelimiterToken(COLON);
    return token == colon_token;
}
