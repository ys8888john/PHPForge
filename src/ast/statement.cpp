#include "statement.h"
#include <iostream>

namespace PHPForge {

// DeclareStmt 实现
void DeclareStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "DeclareStmt: " << directive << " = " << value << "\n";
}

std::string DeclareStmt::toString() const {
    return "DeclareStmt(" + directive + " = " + std::to_string(value) + ")";
}

// ReturnStmt 实现
void ReturnStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "ReturnStmt:\n";
    if (expr) {
        expr->dump(indent + 1);
    }
}

std::string ReturnStmt::toString() const {
    if (expr) {
        return "ReturnStmt(" + expr->toString() + ")";
    }
    return "ReturnStmt()";
}

// EchoStmt 实现
void EchoStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "EchoStmt:\n";
    if (expr) {
        expr->dump(indent + 1);
    }
}

std::string EchoStmt::toString() const {
    return "EchoStmt(" + expr->toString() + ")";
}

// ExpressionStmt 实现
void ExpressionStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "ExpressionStmt:\n";
    if (expr) {
        expr->dump(indent + 1);
    }
}

std::string ExpressionStmt::toString() const {
    return "ExpressionStmt(" + expr->toString() + ")";
}

// BlockStmt 实现
void BlockStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "BlockStmt:\n";
    for (const auto& stmt : statements) {
        stmt->dump(indent + 1);
    }
}

std::string BlockStmt::toString() const {
    std::string result = "BlockStmt[\n";
    for (const auto& stmt : statements) {
        result += "  " + stmt->toString() + "\n";
    }
    result += "]";
    return result;
}

void IfStmt::dump(int indent) const {
    std::cout << indentStr(indent) << "IfStmt:\n";
    if (condition) {
        condition->dump(indent + 1);
    }
    if (thenBranch) {
        thenBranch->dump(indent + 1);
    }
    if (elseBranch) {
        elseBranch->dump(indent + 1);
    }
}

std::string IfStmt::toString() const {
    std::string result = "IfStmt(condition: " + condition->toString() + ", then: " + thenBranch->toString();
    if (elseBranch) {
        result += ", else: " + elseBranch->toString();
    }
    result += ")";
    return result;
}   

} // namespace PHPForge