#ifndef LEXER_H
#define LEXER_H
namespace PHPForge {
class Lexer {
public:
    explicit Lexer(const std::string& source);

    // code to token
    std::vector<Token> tokenize();

    int getCurrentLine() const { return currentLine; }
    int getCurrentColumn() const { return currentColumn; }
    int getCurrentPosition() const { return currentPos; }

private:
    // auxiliary method
    Token readToken();
    Token readWhitespace();
    Token readComment();
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

private:
    std::string source;
    size_t sourceLength;
    size_t currentPos = 0;
    int currentLine = 0;
    int currentColumn = 1;

    struct State {
        size_t pos;
        int line;
        int column;
    };
    State savedSate;
};
}
#endif