#include "lexer.h"
#include <stdexcept>
#include <iostream>

namespace PHPForge {

Lexer::Lexer(const std::vector<char>& source) : source(source),  sourceLength(source.size()){}

// 辅助方法实现
char Lexer::peek(int offset) const {
    size_t pos = currentPos + offset;
    return pos < sourceLength ? source[pos] : '\0';
}

char Lexer::advance() {
    if (currentPos >= sourceLength) {
        return '\0';
    }

    char c = source[currentPos];
    currentPos++;

    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }
    return c;
}

bool Lexer::match(const std::string& str) {
    for (int i; i < str.length(); i++) {
        if (peek(i) != str[i]) {
            return false;
        }
    }

    return true;
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool Lexer::isHexDigit(char c) const {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    int line = currentLine;
    int column = currentColumn;
    int pos = currentPos;

    // 调整位置（因为Token已经消费）
    for (char c : value) {
        if (c == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    return Token(type, value, currentLine, currentColumn, currentPos - value.length());
}

Token Lexer::makeToken(TokenType type, char value) {
    return Token(type, std::string(1, value), currentLine, currentColumn, currentPos - 1);
}

// Token处理实现
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (currentPos < sourceLength) {
        try {
            Token token = readToken();
            tokens.push_back(token);

            // 遇到终止符就停止
            if (token.getType() == TokenType::T_EOF) {
                break;
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    // 确保最后是EOF
    if (tokens.size() > 0) {
        auto lastToken = tokens[tokens.size() - 1];
        if(lastToken.getType() != TokenType::T_EOF) {
            tokens.push_back(makeToken(TokenType::T_EOF, ""));
        }
    }

    return tokens;   
}

Token Lexer::readToken() {
    char c = peek();

    // 文件末尾
    if (c == '\0') {
        return makeToken(TokenType::T_EOF, "");
    }

    // 跳过空白字符
    if (isWhitespace(c)) {
        return readWhitespace();
    }

    // 处理注释
    if (c == '/' && peek(1) == '/') {
        return readLineComment();
    }

    // 处理PHP开放标签
    if (c == '<' && match("<?php")) {
        return readOpenTag();
    }

    // 处理数字
    if (isdigit(c) || (c == '.' && isdigit(peek(1)))) {
        return readNumber();
    }

    // 处理字符串
    // if (c == '"' || c == '\'') {
    //     return readString();
    // }

    // 处理变量（$开头）
    if (c == '$') {
        return readVariable();
    }

    // 处理标识符和关键字
    if (isAlpha(c) || c == '_') {
        return readIdentifierOrKeyword();
    }

    // 处理运算符和分隔符
    return readOperatorOrDelimiter();
}

Token Lexer::readWhitespace() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;
    std::string value;
    while (currentPos < sourceLength && isWhitespace(peek()))
    {
        char c = advance();
        value += c;
        if (c == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
    }

    return Token(TokenType::T_WHITESPACE, value, startLine, startColumn, startPos);
}

Token Lexer::readLineComment() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;

    // 跳过 //
    advance();
    advance();

    std::string value = "//";
    while (currentPos < sourceLength && peek() != '\n') {
        value += advance();
    }

    return Token(TokenType::T_WHITESPACE, value, startLine, startColumn, startPos);
}

Token Lexer::readNumber() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;

    std::string value;
    bool has_dol = false;

    while (currentPos < sourceLength) {
        char c = peek();
        if (isdigit(c)) {
            value += advance();
        } else if (c == '.' && !has_dol) {
            // 检查下一个字符是否为数字
            if (isdigit(peek(1))) {
                has_dol = true;
                value += advance();
            } else {
                break;
            }
        } else if ((c == 'e' || c == 'E') && (peek(1) == '+' || peek(1) == '-' || isdigit(peek(1)))) {
            // 科学计数法
            value += advance(); // e或E
            if (peek() == '+' || peek() == '-') {
                value += advance();
            } 

        } else {
            break;
        }
    }
    return Token(TokenType::T_NUMBER, value, startLine, startColumn, startPos);
}

Token Lexer::readVariable() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;

    // 吃掉$
    char c = advance();
    std::string value = "$";

    // 变量名必须以字符或下划线开头
    if (currentPos < sourceLength) {
        char firstChar = peek();
        if (!isAlpha(firstChar) && firstChar != '_') {
            throw std::invalid_argument("无效的变量名，行：" + std::to_string(startLine) + ", 列：" + std::to_string(startColumn));
        }
    }

    // 读取变量名
    while (currentPos < sourceLength) {
        char c = peek();
        if (isAlphaNumeric(c) || c == '_') {
            value += advance();
        } else {
            break;
        }
    }

    return Token(TokenType::T_VARIABLE, value, startLine, startColumn, startPos);
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;

    std::string value;
    value += advance();

    while (currentPos < sourceLength) {
        char c = peek();
        if (isAlphaNumeric(c) || c == '_') {
            value += advance();
        } else {
            break;
        }
    }

    // 判断是否为关键字
    TokenType type = isKeyword(value) ? keywordToTokenType(value) : TokenType::T_IDENTIFIER;
    return Token(type, value, startLine, startColumn, startPos);

}

Token Lexer::readOperatorOrDelimiter() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;

    char c = advance();

    switch (c) {
        case '+':
            return makeToken(TokenType::T_ADD, c);
        case '(':
            return makeToken(TokenType::T_LPAREN, c);
        case ')':
            return makeToken(TokenType::T_RPAREN, c);
        case '{':
            return makeToken(TokenType::T_LBRACE, c);
        case '}':
            return makeToken(TokenType::T_RBRACE, c);
        case ':':
            return makeToken(TokenType::T_COLON, c);
        case ';':
            return makeToken(TokenType::T_SEMICOLON, c);
        case ',':
            return makeToken(TokenType::T_COMMA, c);
        case '=':
            return makeToken(TokenType::T_ASSIGN, c);
        case '?':
            if (peek() == '>') {
                advance();
                return Token(TokenType::T_EOF, "?>", startLine, startColumn, startPos);
            }
            return Token(TokenType::T_UNKNOWN, "?", startLine, startColumn, startPos);
        default:
            // 未知字符
            std::string char_str(1, c);
            return Token(TokenType::T_UNKNOWN, char_str, startLine, startColumn, startPos);
    }
}

Token Lexer::readOpenTag() {
    int startLine = currentLine;
    int startColumn = currentColumn;
    size_t startPos = currentPos;
    
    // 跳过"<?php"
    advance(); // <
    advance(); // ?
    advance(); // p
    advance(); // h
    advance(); // p

    return Token(TokenType::T_OPEN_TAG, "<?php", startLine, startColumn, startPos);

}

} // namespace PHPForge