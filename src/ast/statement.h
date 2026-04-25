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

class IfStmt : public ASTNode {
public:
    IfStmt(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, std::unique_ptr<ASTNode> elseBranch, int line, int column) :
    ASTNode(ASTNodeType::IF_STMT, line, column), condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)){}

    void dump(int indent) const override;
    std::string toString() const override;

    ASTNode* getCondition() const { return condition.get(); }
    ASTNode* getThenBranch() const { return thenBranch.get(); }
    ASTNode* getElseBranch() const { return elseBranch.get(); }
private:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
};
class ForStmt : public ASTNode {
public:
    ForStmt(std::unique_ptr<ASTNode> init,
            std::unique_ptr<ASTNode> condition,
            std::unique_ptr<ASTNode> update,
            std::unique_ptr<ASTNode> body,
            int line, int column) :
    ASTNode(ASTNodeType::FOR_STMT, line, column),
    init(std::move(init)), condition(std::move(condition)),
    update(std::move(update)), body(std::move(body)) {}

    ASTNode* getInit() const { return init.get(); }
    ASTNode* getCondition() const { return condition.get(); }
    ASTNode* getUpdate() const { return update.get(); }
    ASTNode* getBody() const { return body.get(); }

    void dump(int indent) const override;
    std::string toString() const override;
private:
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> update;
    std::unique_ptr<ASTNode> body;
};

} // namespace PHPForge

#endif // STATEMENT_H