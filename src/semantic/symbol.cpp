#include "symbol.h"
#include "../ast/ASTNode.h"
#include <iostream>

namespace PHPForge {

void VariableSymbol::dump(int indent) const {
    std::cout << std::string(indent * 2, ' ') << "Variable: " << name;
    if (!type.empty()) {
        std::cout << " : " << type;
    }
    std::cout << std::endl;
}

void FunctionSymbol::dump(int indent) const {
    std::cout << std::string(indent * 2, ' ') << "Function: " << name;
    if (!type.empty()) {
        std::cout << " -> " << type;
    }
    std::cout << std::endl;
    
    for (size_t i = 0; i < parameterNames.size(); ++i) {
        std::cout << std::string((indent + 1) * 2, ' ') 
                  << "Parameter: " << parameterNames[i]
                  << " : " << parameterTypes[i] << std::endl;
    }
}

void ParameterSymbol::dump(int indent) const {
    std::cout << std::string(indent * 2, ' ') << "Parameter: " << name;
    if (!type.empty()) {
        std::cout << " : " << type;
    }
    std::cout << std::endl;
}

} // namespace PHPForge
