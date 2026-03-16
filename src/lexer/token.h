#ifndef TOKEN_H
#define TOKEN_H

#include "tokenType.h"

namespace PHPForge {
class Token {
public:
    Token(TokenType type, const std::string& value, int line, int column, int position)
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
        return ::PHPForge::isKeyword(value);
    }

    bool isOperator() const {
        return ::PHPForge::isOperator(type);
    }
    
private:
    TokenType type;
    std::string value;
    int line;
    int column;
    int position;
};

} // namespace PHPForge

#endif  // TOKEN_H
