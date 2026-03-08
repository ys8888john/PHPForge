#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include "tokenType.h"
#include "token.h"

namespace PHPForge {
class Lexer {
public:
    explicit Lexer(const std::vector<char>& source);

    // code to token
    std::vector<Token> tokenize();

    void dump(const std::vector<Token> &tokens) const;

    int getCurrentLine() const { return currentLine; }
    int getCurrentColumn() const { return currentColumn; }
    int getCurrentPosition() const { return currentPos; }

private:
    // auxiliary method
    Token readToken();
    Token readWhitespace();
    Token readLineComment();
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
    Token readVariable();
    Token readOperatorOrDelimiter();
    Token readOpenTag();

    // character processing method
    char peek(int offset = 0) const;
    char advance();
    bool match(const std::string& str);
    void skipWhitespace();

    // character judgment method
    // 检查字符是否是字母（大小写A-Z、a-z）
    bool isAlpha(char c) const;

    // 检查字符是否是数字（0-9）
    bool isDigit(char c) const;

    // 检查字符是否是字母或数字（A-Z、a-z、0-9）
    bool isAlphaNumeric(char c) const;

    // 检查字符是否是空白字符（空格、制表符、换行等）
    bool isWhitespace(char c) const;

    // 检查字符是否是十六进制数字（0-9、A-F、a-f）
    bool isHexDigit(char c) const;

    // 创建token
    Token makeToken(TokenType type, char value);

private:
    std::vector<char> source;
    size_t sourceLength;
    size_t currentPos = 0;
    int currentLine = 0;
    int currentColumn = 1;
}; 

} // namespace PHPForge

#endif // LEXER_H