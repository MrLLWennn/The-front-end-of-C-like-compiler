//
// 创建者：86198；日期：2024/10/20。
//

#include "keyword_token.h"




bool my_compiler_token::KeywordToken::is_keyword(const string &value) noexcept
{
    return keywords_.find(value) != keywords_.end();
}

bool my_compiler_token::KeywordToken::is_if_token(const Token& token) noexcept
{
    KeywordToken if_token = KeywordToken(IF);
    return token == if_token;
}

bool my_compiler_token::KeywordToken::is_else_token(const Token& token) noexcept
{
    KeywordToken else_token = KeywordToken(ELSE);
    return token == else_token;
}

bool my_compiler_token::KeywordToken::is_return_token(const Token& token) noexcept
{
    KeywordToken return_token = KeywordToken(RETURN);
    return token == return_token;
}

bool my_compiler_token::KeywordToken::is_break_token(const Token &token) noexcept
{
    KeywordToken break_token = KeywordToken(BREAK);
    return token == break_token;
}

bool my_compiler_token::KeywordToken::is_for_token(const Token &token) noexcept
{
    KeywordToken for_token = KeywordToken(FOR);
    return token == for_token;
}

bool my_compiler_token::KeywordToken::is_void_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken void_token = KeywordToken(VOIDD);
    return token == void_token;
}

bool my_compiler_token::KeywordToken::is_float_token(const my_compiler_token::Token &token) noexcept
{
   KeywordToken string_token = KeywordToken(FLOAT);
   return token == string_token;
}

bool my_compiler_token::KeywordToken::is_while_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken while_token = KeywordToken(WHILE);
    return token == while_token;
}


bool my_compiler_token::KeywordToken::is_integer_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken integer_token = KeywordToken(INTEGER);
    return token == integer_token;
}



set<string> my_compiler_token::KeywordToken::keywords_ = {
        IF,
        WHILE,
        FOR,
        RETURN,
        BREAK,
        CONTINUE,
        ELSE,
        VOIDD,
        FLOAT,
        INTEGER,
        CONS,
        VOIDD,
        CASE,
        SWITCH,
        BOOL_,
        DOUBLE,
        CHAR_,
        STRUCT,
        DEFAULT_
};

bool my_compiler_token::KeywordToken::is_switch_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken switch_token = KeywordToken(SWITCH);
    return token == switch_token;
}

bool my_compiler_token::KeywordToken::is_case_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken case_token = KeywordToken(CASE);
    return token == case_token;
}

bool my_compiler_token::KeywordToken::is_const_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken const_token = KeywordToken(CONS);
    return token == const_token;
}

bool my_compiler_token::KeywordToken::is_double_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken double_token = KeywordToken(DOUBLE);
    return token == double_token;
}


bool my_compiler_token::KeywordToken::is_struct_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken struct_token = KeywordToken(STRUCT);
    return token == struct_token;
}

bool my_compiler_token::KeywordToken::is_include_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken include_token = KeywordToken(INCLUDE);
    return token == include_token;
}

bool my_compiler_token::KeywordToken::is_bool_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken bool_token = KeywordToken(BOOL_);
    return token == bool_token;
}

bool my_compiler_token::KeywordToken::is_default_token(const my_compiler_token::Token &token) noexcept
{
    KeywordToken default_token = KeywordToken(DEFAULT_);
    return token == default_token;
}





