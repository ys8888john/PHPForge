#include "functionDecl.h"
#include "statement.h"
#include <iostream>

namespace PHPForge {
// Parameter
void Parameter::dump(int indent) const {
    std::cout << indentStr(indent)
              << "Parameter: " << name
              << " : " << typeHint << "\n";
}

std::string Parameter::toString() const {
    return "Parameter(" + name + " : " + typeHint + ")";
}

// FunctionDecl
void FunctionDecl::dump(int indent) const {
    std::cout << indentStr(indent) << "Function: " << name << " -> " << returnType << "\n";

    std::cout << indentStr(indent) << " Parameters:\n";
    for (const auto& param : params) {
        param->dump(indent + 2);
    }

    std::cout << indentStr(indent) << " Body:\n";
    if (body) {
        body->dump(indent + 2);
    }
}

std::string FunctionDecl::toString() const {
    std::string result = "FunctionDecl(" + name + ", returns: " + returnType + ")";
    result += " params=[";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) result += ", ";
        result += params[i]->toString();
    }
    result += "]";
    return result;
}

} // namespace PHPForge