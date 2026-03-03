#ifndef TOKEN_H
#define TOKEN_H

#include "TokenType.hpp"

namespace PHPForge {
class Token {
public:
    Token(TokenType type, const std::string& value, int line, int column, int position) :
        : type(type), value(value), line(line), column(column), position(position) {}
    
    TokenType getType() const { return type; }
    const std::string& getValue() const { return value; }
    int getLine() const { return line; }
    int getColumn() const { return column; }
    int getPosition() const { return position; }

    bool is(TokenType ty) const { return ty == type; }
    bool is(TokenType ty, const std::string& val) const {
        return ty == type && val == value;
    }

    bool isKeyword() const {
        return isKeyword(value);
    }

    bool isOperator() const {
        return isOperator(type);
    }
    
private:
    TokenType type;
    std::string value;
    int line;
    int column;
    int position;
}
}
#endif