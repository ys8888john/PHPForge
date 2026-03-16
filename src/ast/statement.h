#ifndef STATEMENT_H
#define STATEMENT_H

#include "ASTNode.h"
#include <vector>
#include <memory>
#include <string>

namespace PHPForge {

// Declare语句节点
class DeclareStmt : public ASTNode {
public:
    DeclareStmt(const std::string& directive, int value, int line, int column) :
    ASTNode(ASTNodeType::DECLARE_STMT, line, column), directive(directive), value(value) {}

    const std::string& getDirective() const { return directive; }
    int getValue() const { return value; }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::string directive;
    int value;
};

// 返回语句节点
class ReturnStmt : public ASTNode {
public:
    ReturnStmt(std::unique_ptr<ASTNode> expr, int line, int column) :
    ASTNode(ASTNodeType::RETURN_STMT, line, column), expr(std::move(expr)) {}

    ASTNode* getExpr() const { return expr.get(); }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> expr;
};

// Echo语句节点
class EchoStmt : public ASTNode {
public:
    EchoStmt(std::unique_ptr<ASTNode> expr, int line, int column) :
    ASTNode(ASTNodeType::ECHO_STMT, line, column), expr(std::move(expr)) {}

    ASTNode* getExpr() const { return expr.get(); }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> expr;
};

// 表达式语句节点
class ExpressionStmt : public ASTNode {
public:
    ExpressionStmt(std::unique_ptr<ASTNode> expr, int line, int column) :
    ASTNode(ASTNodeType::EXPRESSION_STMT, line, column), expr(std::move(expr)) {}

    ASTNode* getExpr() const { return expr.get(); }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> expr;
};

// 代码块节点
class BlockStmt : public ASTNode {
public:
    BlockStmt(int line, int column) :
    ASTNode(ASTNodeType::BLOCK_STMT, line, column) {}

    void addStatement(std::unique_ptr<ASTNode> stmt) {
        statements.push_back(std::move(stmt));
    }

    const std::vector<std::unique_ptr<ASTNode>>& getStatements() const { return statements; }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::vector<std::unique_ptr<ASTNode>> statements;
};

} // namespace PHPForge

#endif // STATEMENT_H