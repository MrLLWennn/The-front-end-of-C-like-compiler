//
// 创建者：86198；日期：2024/10/20。
//

#ifndef MY_COMPILER_TOKEN_H
#define MY_COMPILER_TOKEN_H


#include"../header_files.h"


namespace my_compiler_token
{

    enum TokenType
    {
        KEYWORD = 1,
        IDENTIFIER = 2,
        NUMBER = 3,
        DELIMITER = 4,
        OPERATOR = 5,
        END = 6,
        DEFAULT = 7,
        BOOLEAN,
        ERR = 9
    };


    string get_token_name(TokenType type);


    //词法单元
    class Token
    {
    public:

        Token()
                :token_name_(TokenType::DEFAULT), attribute_value_(string()), line_(1)
        {
        }

        Token(const string& value, const size_t& line)
        :token_name_(TokenType::DEFAULT), attribute_value_(value), line_(line)
        {
        }

        explicit Token(const string& value)
        :token_name_(TokenType::DEFAULT), attribute_value_(value), line_(1)
        {
        }

        Token(TokenType type, const string& value)
                :token_name_(type), attribute_value_(value), line_(1)
        {
        }

        Token(TokenType type, const string& value, const size_t& line)
                :token_name_(type), attribute_value_(value), line_(line)
        {
        }

        Token(TokenType type, const char& value)
                :token_name_(type), attribute_value_(string(1, value)),line_(1)
        {
        }

        Token(TokenType type, const char& value, const size_t& line)
        :token_name_(type), attribute_value_(string(1, value)), line_(line)
        {

        }


    public:

        void set_attribute_value(const string& value)
        {
            attribute_value_ = value;
        }

        void set_attribute_value(const char& value)
        {
            attribute_value_ = string(1, value);
        }

        [[nodiscard]] bool is_eof() const noexcept
        {
            return this->token_name_ == TokenType::END;
        }

        [[nodiscard]] bool is_identifier() const noexcept
        {
            return this->token_name_ == TokenType::IDENTIFIER;
        }

        [[nodiscard]] bool is_number() const noexcept
        {
            return this->token_name_ == TokenType::NUMBER;
        }

        [[nodiscard]] bool is_keyword() const noexcept
        {
            return this->token_name_ == TokenType::KEYWORD;
        }

        [[nodiscard]] bool is_operator() const noexcept
        {
            return this->token_name_ == TokenType::OPERATOR;
        }

        [[nodiscard]] bool is_boolean() const noexcept
        {
            return this->token_name_ == TokenType::BOOLEAN;
        }

        [[nodiscard]] TokenType get_token_name_() const noexcept
        {
            return this->token_name_;
        }

        [[nodiscard]] size_t get_line() const noexcept
        {
            return this->line_;
        }

        void error()
        {
            token_name_ = ERR;
            attribute_value_ = "error token";
        }

        [[nodiscard]] string get_attribute_value() const noexcept
        {
            return this->attribute_value_;
        }

    private:
        //类型
        TokenType token_name_;
        //本身
        string attribute_value_;
        //所在行
        size_t line_;

    private:


        friend std::ostream& operator<<(std::ostream& stream, const Token& token)
        {
            string token_type = get_token_name(token.token_name_);
            return stream << token_type << "(\"" << token.attribute_value_ << "\") ";
        }

        friend bool operator==(const Token& token1, const Token& token2)
        {
            return token1.token_name_ == token2.token_name_ && token1.attribute_value_ == token2.attribute_value_;
        }

        friend bool operator!=(const Token& token1, const Token& token2)
        {
            return token1.token_name_ != token2.token_name_ || token1.attribute_value_ != token2.attribute_value_;
        }

        friend bool operator<(const Token& token1, const Token& token2)
        {
            return token1.attribute_value_ < token2.attribute_value_;
        }


    };


}

#endif //MY_COMPILER_TOKEN_H
