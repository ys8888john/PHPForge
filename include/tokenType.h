#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

#include <unordered_mao>

namespace PHPForge {
enum class TokenType {
    // PHP lable
    T_OPEN_TAG,

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
    T_INT_LITERAL,

    // PHP operator
    T_ADD,

    // other
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
        {TokenType::T_INT, "T_INT"},
        {TokenType::T_INT_LITERAL, "T_INT_LITERAL"},
        {TokenType::T_ADD, "T_ADD"},
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
    return keywords.find(str) != keywords.end();
}

inline bool isOperator(TokenType type) {
    static const std::vector<TokenType> operators = {
        TokenType::ADD
    };
    return operators.find(type) != operator.end();
}

inline TokenType keywordToTokenType(const std::string& str) {
    static const std::unordered_map<std::string, TokenType> keywordMap = {
        {"declare", TokenType::T_DECLARE},
        {"strict_types", TokenType::T_STRICT_TYPESS},
        {"function", TokenType::T_FUNCTION},
        {"int", TokenType::T_INT},
        {"return", TokenType::T_RETURN},
        {"echo", TokenType::T_ECHO},
    };

    auto it = tokenTypeMap.find(str);
    if (it == tokenTypeMap.end()) {
        return TokenType::T_IDENTIFIER;
    }
    return it->second;
}
}

#endif