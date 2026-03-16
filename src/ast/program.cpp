#include "program.h"
#include <iostream>

namespace PHPForge {

void Program::dump(int indent) const {
    std::cout << indentStr(indent) << "Program:" << std::endl;
    for (const auto& statement : statements) {
        statement->dump(indent + 1);
    }
}

std::string Program::toString() const {
    std::string result = "Program {\n";
    for (const auto& statement : statements) {
        result += " " + statement->toString() + "\n";
    }
    result += "}";
    return result;
}

} // namespace PHPForge
    