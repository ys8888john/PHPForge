#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "value.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace PHPForge {

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr)
        : enclosing(enclosing) {}

    // 定义变量
    void define(const std::string& name, const Value& value) {
        values[name] = value;
    }

    // 获取变量
    Value get(const std::string& name);
    
    // 获取变量（带位置信息，用于错误报告）
    Value getAt(const std::string& name, int line, int column);

    // 修改变量
    void assign(const std::string& name, const Value& value);
    
    // 修改变量（带位置信息）
    void assignAt(const std::string& name, const Value& value, int line, int column);

    // 检查变量是否存在（递归检查所有外层环境）
    bool contains(const std::string& name) const {
        if (values.find(name) != values.end()) {
            return true;
        }
        if (enclosing) {
            return enclosing->contains(name);
        }
        return false;
    }

    // 获取外层环境
    std::shared_ptr<Environment> getEnclosing() const { return enclosing; }

private:
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;  // 外层作用域
};

} // namespace PHPForge

#endif // ENVIRONMENT_H
