//
// Created by 86198 on 2024/10/20.
//

#ifndef MY_COMPILER_KEYWORD_TOKEN_H
#define MY_COMPILER_KEYWORD_TOKEN_H


#include"../token.h"


namespace my_compiler_token
{

    const string IF = "if";
    const string WHILE = "while";
    const string FOR = "for";
    const string RETURN = "return";
    const string BREAK = "break";
    const string ELSE = "else";
    const string VOIDD = "void";
    const string FLOAT = "float";
    const string INTEGER = "int";
    const string CASE = "case";
    const string SWITCH = "switch";
    const string CONS = "const";
    const string DOUBLE = "double";
    const string STRUCT = "struct";
    const string INCLUDE = "include";
    const string BOOL_ = "bool";
    const string DEFAULT_ = "default";


    class KeywordToken : public Token
    {
    public:
        KeywordToken(const string& value, const size_t& line)
        :Token(TokenType::KEYWORD, value, line)
        {
        }

        explicit KeywordToken(const string& value)
        :Token(TokenType::KEYWORD, value)
        {
        }


    public:


        [[nodiscard("no use return")]] static bool is_keyword(const string& value) noexcept;
        [[nodiscard("no use return")]] static bool is_if_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_integer_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_while_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_else_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_return_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_break_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_void_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_float_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_for_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_switch_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_case_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_const_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_double_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_struct_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_include_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_bool_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_default_token(const Token& token) noexcept;


    private:
        static set<string> keywords_;

        friend std::ostream& operator<<(std::ostream& out, const KeywordToken& rhs)
        {
            string token_type = get_token_name(rhs.get_token_name_());
            if(is_if_token(rhs))
            {
                return out << token_type << "(\"" << IF << "\")";
            }
            else if(is_else_token(rhs))
            {
                return out << token_type << "(\"" << ELSE << "\")";
            }
            else if(is_while_token(rhs))
            {
                return out << token_type << "(\"" << WHILE << "\")";
            }
            else if(is_return_token(rhs))
            {
                return out << token_type << "(\"" << RETURN << "\")";
            }
            else if(is_break_token(rhs))
            {
                return out << token_type << "(\"" << BREAK << "\")";
            }
            else if(is_for_token(rhs))
            {
                return out << token_type << "(\"" << FOR << "\")";
            }
            else if(is_float_token(rhs))
            {
                return out << token_type << "(\"" << FLOAT << "\")";
            }
            else if(is_void_token(rhs))
            {
                return out << token_type << "(\"" << VOIDD << "\")";
            }
            else if(is_integer_token(rhs))
            {
                return out << token_type << "(\"" << INTEGER << "\")";
            }
            else if(is_switch_token(rhs))
            {
                return out << token_type << "(\"" << SWITCH << "\")";
            }
            else if(is_case_token(rhs))
            {
                return out << token_type << "(\"" << CASE << "\")";
            }
            else if(is_const_token(rhs))
            {
                return out << token_type << "(\"" << CONS << "\")";
            }
            else if(is_double_token(rhs))
            {
                return out << token_type << "(\"" << DOUBLE << "\")";
            }
            else if(is_struct_token(rhs))
            {
                return out << token_type << "(\"" << STRUCT << "\")";
            }
            else if(is_include_token(rhs))
            {
                return out << token_type << "(\"" << INCLUDE << "\")";
            }
            else if(is_bool_token(rhs))
            {
                return out << token_type << "(\"" << BOOL_ << "\")";
            }
            else if(is_default_token(rhs))
            {
                return out << token_type << "(\"" << DEFAULT_ << "\")";
            }
            else
            {
                return out << "[Error]" << ERROR << endl;
            }
        }
    };


}

#endif //MY_COMPILER_KEYWORD_TOKEN_H
