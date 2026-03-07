#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

namespace PHPForge {
enum class TokenType {
    // PHP lable
    T_OPEN_TAG, // <?php
    T_EOF, // ?>

    // PHP keywords
    T_DECLARE,
    T_STRICT_TYPES,
    T_FUNCTION,
    T_RETURN,
    T_ECHO,

    // PHP identifire
    T_IDENTIFIER, // like function name or constant name
    T_VARIABLE, // variable name, $ begin

    // PHP type keywords
    T_INT,

    // PHP literal
    T_NUMBER,

    // PHP operator
    T_ADD,

    // PHP punctuation
    T_LPAREN,      // (
    T_RPAREN,      // )
    T_LBRACE,      // {
    T_RBRACE,      // }
    T_COLON,       // :
    T_SEMICOLON,   // ;
    T_COMMA,       // ,
    T_ASSIGN,      // =

    // other
    T_WHITESPACE,
    T_UNKNOWN
};

inline std::string tokenTypeToString(TokenType type) {
    static const std::unordered_map<TokenType, std::string> tokenTypeMap = {
        {TokenType::T_OPEN_TAG, "T_OPEN_TAG"},
        {TokenType::T_DECLARE, "T_DECLARE"},
        {TokenType::T_STRICT_TYPES, "T_STRICT_TYPES"},
        {TokenType::T_FUNCTION, "T_FUNCTION"},
        {TokenType::T_RETURN, "T_RETURN"},
        {TokenType::T_ECHO, "T_ECHO"},
        {TokenType::T_IDENTIFIER, "T_IDENTIFIER"},
        {TokenType::T_VARIABLE, "T_VARIABLE"},
        {TokenType::T_INT, "T_INT"},
        {TokenType::T_NUMBER, "T_NUMBER"},
        {TokenType::T_ADD, "T_ADD"},
        {TokenType::T_LPAREN, "T_LPAREN"},
        {TokenType::T_RPAREN, "T_RPAREN"},
        {TokenType::T_LBRACE, "T_LBRACE"},
        {TokenType::T_RBRACE, "T_RBRACE"},
        {TokenType::T_COLON, "T_COLON"},
        {TokenType::T_SEMICOLON, "T_SEMICOLON"},
        {TokenType::T_COMMA, "T_COMMA"},
        {TokenType::T_ASSIGN, "T_ASSIGN"},
        {TokenType::T_EOF, "T_EOF"},
        {TokenType::T_WHITESPACE, "T_WHITESPACE"},
        {TokenType::T_UNKNOWN, "T_UNKNOWN"}
    };

    auto it = tokenTypeMap.find(type);
    if (it == tokenTypeMap.end()) {
        return "T_UNKNOWN";
    }
    return it->second;
}

inline bool isKeyword(const std::string& str) {
    static const std::vector<std::string> keywords = {
        "declare", "strict_types", "function", "int", "return", "echo"
    };
    return std::find(keywords.begin(), keywords.end(), str) != keywords.end();
}

inline bool isOperator(TokenType type) {
    static const std::vector<TokenType> operators = {
        TokenType::T_ADD
    };

    return std::find(operators.begin(), operators.end(), type) != operators.end();
}

inline TokenType keywordToTokenType(const std::string& str) {
    static const std::unordered_map<std::string, TokenType> keywordMap = {
        {"declare", TokenType::T_DECLARE},
        {"strict_types", TokenType::T_STRICT_TYPES},
        {"function", TokenType::T_FUNCTION},
        {"int", TokenType::T_INT},
        {"return", TokenType::T_RETURN},
        {"echo", TokenType::T_ECHO},
    };

    auto it = keywordMap.find(str);
    if (it == keywordMap.end()) {
        return TokenType::T_IDENTIFIER;
    }
    return it->second;
}

} // namespace PHPForge

#endif  // TOKEN_TYPE_H