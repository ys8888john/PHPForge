#include "environment.h"
#include <stdexcept>

namespace PHPForge {

Value Environment::get(const std::string& name) {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    
    // 在当前环境找不到，去外层环境查找
    if (enclosing) {
        return enclosing->get(name);
    }
    
    throw std::runtime_error("Undefined variable: '" + name + "'");
}

Value Environment::getAt(const std::string& name, int line, int column) {
    try {
        return get(name);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            std::string(e.what()) + " at line " + 
            std::to_string(line) + ", column " + std::to_string(column)
        );
    }
}

void Environment::assign(const std::string& name, const Value& value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    
    // 在当前环境找不到，去外层环境赋值
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
    
    throw std::runtime_error("Undefined variable: '" + name + "'");
}

void Environment::assignAt(const std::string& name, const Value& value, int line, int column) {
    try {
        assign(name, value);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            std::string(e.what()) + " at line " + 
            std::to_string(line) + ", column " + std::to_string(column)
        );
    }
}

} // namespace PHPForge
