#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ASTNode.h"
#include <string>
#include <vector>
#include <memory>

namespace PHPForge {
class BinaryExpr : public ASTNode {
public:
    BinaryExpr(std::unique_ptr<ASTNode> left,
        const std::string& op,
        std::unique_ptr<ASTNode> right,
        int line, int column) : ASTNode(ASTNodeType::BINARY_EXPR, line, column),
        left(std::move(left)), op(op), right(std::move(right)) {}
    
    const ASTNode* getLeft() const { return left.get(); }
    const ASTNode* getRight() const { return right.get(); }
    const std::string& getOp() const { return op; }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> left;
    std::string op;
    std::unique_ptr<ASTNode> right;
};

// 函数调用节点
class CallExpr : public ASTNode {
public:
    CallExpr(std::unique_ptr<ASTNode> callee,
        std::vector<std::unique_ptr<ASTNode>> args,
        int line, int column) :
         ASTNode(ASTNodeType::CALL_EXPR, line, column),
         callee(std::move(callee)), args(std::move(args)) {}

    const ASTNode* getCallee() const { return callee.get(); }
    const std::vector<std::unique_ptr<ASTNode>>& getArgs() const { return args; }
    const std::vector<std::unique_ptr<ASTNode>>& getArguments() const { return args; }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> callee;
    std::vector<std::unique_ptr<ASTNode>> args;
};

// 字面量节点
class LiteralExpr : public ASTNode {
public:
    LiteralExpr(const std::string& value, const std::string& type, int line, int column) :
        ASTNode(ASTNodeType::LITERAL, line, column), value(value), type(type) {}
    
    const std::string& getValue() const { return value; }
    const std::string& getType() const { return type; }
    
    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::string value;
    std::string type;
};

// 变量节点
class VariableExpr : public ASTNode {
public:
    VariableExpr(const std::string& name, int line, int column) :
        ASTNode(ASTNodeType::VARIABLE, line, column), name(name) {}
    
    const std::string& getName() const { return name; }
    
    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::string name;
};

// 标识符节点
class IdentifierExpr : public ASTNode {
public:
    IdentifierExpr(const std::string& name, int line, int column) :
        ASTNode(ASTNodeType::IDENTIFIER, line, column), name(name) {}
    
    const std::string& getName() const { return name; }
    
    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::string name;
};

} // namespace PHPForge

#endif // EXPRESSION_H