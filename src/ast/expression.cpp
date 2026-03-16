#include "expression.h"
#include <iostream>

namespace PHPForge {

// BinaryExpr
void BinaryExpr::dump(int indent) const {
    std::cout << indentStr(indent) << "BinaryExpr: " << op << "\n";
    std::cout << indentStr(indent) << "  Left:\n";
    left->dump(indent + 2);
    std::cout << indentStr(indent) << "  Right:\n";
    right->dump(indent + 2);
}

std::string BinaryExpr::toString() const {
    return "BinaryExpr(" + left->toString() + " " + op + " " + right->toString() + ")";
}

// CallExpr
void CallExpr::dump(int indent) const {
    std::cout << indentStr(indent) << "CallExpr:\n";
    std::cout << indentStr(indent) << "  Callee:\n";
    callee->dump(indent + 2);

    std::cout << indentStr(indent) << "  Arguments:\n";
    for (const auto& arg : args) {
        arg->dump(indent + 2);
    }
}

std::string CallExpr::toString() const {
    std::string result = "CallExpr(" + callee->toString() + ", args=[";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) result += ", ";
        result += args[i]->toString();
    }
    result += "])";
    return result;
}

// LiteralExpr
void LiteralExpr::dump(int indent) const {
    std::cout << indentStr(indent) << "Literal: " << value << " (" << type << ")\n";
}

std::string LiteralExpr::toString() const {
    return "Literal(" + value + " : " + type + ")";
}

// Variable
void VariableExpr::dump(int indent) const {
    std::cout << indentStr(indent) << "Variable: " << name << "\n";
}

std::string VariableExpr::toString() const {
    return "Variable(" + name + ")";
}

// Identifier
void IdentifierExpr::dump(int indent) const {
    std::cout << indentStr(indent) << "Identifier: " << name << "\n";
}

std::string IdentifierExpr::toString() const {
    return "Identifier(" + name + ")";
}

} // namespace PHPForge