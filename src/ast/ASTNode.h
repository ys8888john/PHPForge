#ifndef ASTNODE_H
#define ASTNODE_H

#include <string>
#include <memory>
#include <vector>
#include <iostream>

namespace PHPForge {
enum class ASTNodeType {
    // 程序结构
    PROGRAM,

    // 声明
    DECLARE_STMT,
    FUNCTION_DECL,
    VARIABLE_DECL,
    PARAMETER,

    // 语句
    EXPRESSION_STMT,
    RETURN_STMT,
    ECHO_STMT,
    BLOCK_STMT,

    // 表达式
    BINARY_EXPR,
    CALL_EXPR,
    LITERAL,
    VARIABLE,
    IDENTIFIER,

    // 类型
    TYPE_HINT
};

// AST节点基类
class ASTNode {
public:
    ASTNode(ASTNodeType type, int line, int column) : nodeType(type), line(line), column(column) {}
    virtual ~ASTNode() = default;

    ASTNodeType getNodeType() const { return nodeType; }
    int getLine() const { return line; }
    int getColumn() const { return column; }

    virtual void dump(int indent = 0) const = 0;
    virtual std::string toString() const = 0;

    // 类型检查
    bool is(ASTNodeType type) const { return nodeType == type; }

protected:
    ASTNodeType nodeType;
    int line;
    int column;

    //辅助方法：缩进打印
    static std::string indentStr(int indent) {
        return std::string(indent * 2, ' ');
    }
};

using ASTNodePtr = std::unique_ptr<ASTNode>;
using StatementPtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<ASTNode>;

} // namespace PHPForge

#endif // ASTNODE_H