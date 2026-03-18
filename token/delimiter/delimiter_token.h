//
// Created by 86198 on 2024/10/20.
//

#ifndef MY_COMPILER_DELIMITER_TOKEN_H
#define MY_COMPILER_DELIMITER_TOKEN_H


#include"../token.h"


namespace my_compiler_token
{




    //定界符常量
    const char DOT                      = '.';
    const char LEFT_PARENTHESES         = '(';
    const char RIGHT_PARENTHESES        = ')';
    const char COMMA                    = ',';
    const char LEFT_CURLY_BRACE         = '{';
    const char RIGHT_CURLY_BRACE        = '}';
    const char SEMI_COLON               = ';';
    const char LEFT_SQUARE_BRACKET      = '[';
    const char RIGHT_SQUARE_BRACKET     = ']';
    const char COLON                    = ':';


    class DelimiterToken : public Token
    {
    public:

        DelimiterToken(const string& value, const size_t& line)
        :Token(TokenType::DELIMITER ,value ,line)
        {
        }


        explicit DelimiterToken(const char& value)
        :Token(TokenType::DELIMITER, value)
        {
        }

        DelimiterToken(const char& value, const size_t& line)
        :Token(TokenType::DELIMITER, value, line)
        {
        }



    public:

        [[nodiscard("no use return")]] static bool is_delimiter(const char& ch);
        [[nodiscard("no use return")]] static bool is_dot_token(const Token& token);
        [[nodiscard("no use return")]] static bool is_left_parentheses_token(const Token& token);
        [[nodiscard("no use return")]] static bool is_right_parentheses_token(const Token& token);
        [[nodiscard("no use return")]] static bool is_comma_token(const Token& token);
        [[nodiscard("no use return")]] static bool is_left_curly_brace(const Token& token);
        [[nodiscard("no use return")]] static bool is_right_curly_brace(const Token& token);
        [[nodiscard("no use return")]] static bool is_semicolon_token(const Token& token);
        [[nodiscard("no use return")]] static bool is_left_square_bracket(const Token& token);
        [[nodiscard("no use return")]] static bool is_right_square_bracket(const Token& token);
        [[nodiscard("no use return")]] static bool is_colon(const Token& token);


    private:
        //定界符集合
        static std::set<char> delimiters_;

        friend std::ostream& operator<<(std::ostream& out, const DelimiterToken& rhs)
        {
            string token_type = get_token_name(rhs.get_token_name_());
            if(is_dot_token(rhs))
            {
                return out << token_type << "(\"" << DOT << "\")";
            }
            else if(is_comma_token(rhs))
            {
                return out << token_type << "(\"" << COMMA << "\")";
            }
            else if(is_left_parentheses_token(rhs))
            {
                return out << token_type << "(\"" << LEFT_PARENTHESES << "\")";
            }
            else if(is_right_parentheses_token(rhs))
            {
                return out << token_type << "(\"" << RIGHT_PARENTHESES << "\")";
            }
            else if(is_left_curly_brace(rhs))
            {
                return out << token_type << "(\"" << LEFT_CURLY_BRACE << "\")";
            }
            else if(is_right_curly_brace(rhs))
            {
                return out << token_type << "(\"" << RIGHT_CURLY_BRACE << "\")";
            }
            else if(is_semicolon_token(rhs))
            {
                return out << token_type << "(\"" << SEMI_COLON << "\")";
            }
            else if(is_left_square_bracket(rhs))
            {
                return out << token_type << "(\"" << LEFT_SQUARE_BRACKET << "\")";
            }
            else if(is_right_square_bracket(rhs))
            {
                return out << token_type << "(\"" << RIGHT_SQUARE_BRACKET << "\")";
            }
            else if(is_colon(rhs))
            {
                return out << token_type << "(\"" << COLON << "\")";
            }
            else
            {
                return out << "[Error]" << ERROR << endl;
            }
        }
    };
}


#endif //MY_COMPILER_DELIMITER_TOKEN_H
