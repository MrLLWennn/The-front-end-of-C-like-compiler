//
// 创建者：86198；日期：2024/10/20。
//

#ifndef MY_COMPILER_OPERATOR_TOKEN_H
#define MY_COMPILER_OPERATOR_TOKEN_H


#include"../token.h"


namespace my_compiler_token
{


    const char ADDITION = '+';
    const char SUBTRACTION = '-';
    const char MULTIPLICATION = '*';
    const char DIVISION = '/';
    const char LESS_THAN = '<';
    const char GREATER_THAN = '>';
    const char ASSIGNMENT = '=';
    const char POWER = '^';
    const char NEGATION = '!';


    class OperatorToken : public Token
    {
    public:

        OperatorToken(const string& value, const size_t& line)
        :Token(TokenType::OPERATOR, value, line)
        {
        }

        explicit OperatorToken(const char& value, const size_t& line = 1)
        :Token(TokenType::OPERATOR, value, line)
        {
        }



    public:

        [[nodiscard("no use return")]] static bool is_operator(const char& ch) noexcept;
        [[nodiscard("no use return")]] static bool is_addition_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_subtraction_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_multiplication_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_division_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_less_than_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_greater_than_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_assignment_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_power_token(const Token& token) noexcept;
        [[nodiscard("no use return")]] static bool is_negation_token(const Token& token) noexcept;

    private:
        static set<char> operators_;

        friend std::ostream& operator<<(std::ostream& out, const OperatorToken& rhs)
        {
            string token_type = get_token_name(rhs.get_token_name_());
            if(is_addition_token(rhs))
            {
                return out << token_type << "(\"" << ADDITION << "\")";
            }
            else if(is_subtraction_token(rhs))
            {
                return out << token_type << "(\"" << SUBTRACTION << "\")";
            }
            else if(is_multiplication_token(rhs))
            {
                return out << token_type << "(\"" << MULTIPLICATION<< "\")";
            }
            else if(is_division_token(rhs))
            {
                return out << token_type << "(\"" << DIVISION << "\")";
            }
            else if(is_less_than_token(rhs))
            {
                return out << token_type << "(\"" << LESS_THAN << "\")";
            }
            else if(is_greater_than_token(rhs))
            {
                return out << token_type << "(\"" << GREATER_THAN << "\")";
            }
            else if(is_assignment_token(rhs))
            {
                return out << token_type << "(\"" <<ASSIGNMENT << "\")";
            }
            else if(is_power_token(rhs))
            {
                return out << token_type << "(\"" << POWER<< "\")";
            }
            else if(is_negation_token(rhs))
            {
                return out << token_type << "(\"" << NEGATION << "\")";
            }
            else
            {
                return out << "[Error] unsupported Operator token: " << rhs.get_attribute_value();
            }
        }
    };


}


#endif //MY_COMPILER_OPERATOR_TOKEN_H
