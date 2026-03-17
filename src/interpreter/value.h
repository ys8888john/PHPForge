#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <functional>

namespace PHPForge {

// 前置声明
class FunctionDecl;
class Environment;

// 值的类型
enum class ValueType {
    NULL_TYPE,
    BOOL,
    INT,
    FLOAT,
    STRING,
    FUNCTION,
    RETURN_VALUE  // 用于处理return语句的特殊包装类型
};

// 运行时值
class Value {
public:
    Value() : type(ValueType::NULL_TYPE) {}
    explicit Value(bool val) : type(ValueType::BOOL), data(val) {}
    explicit Value(int val) : type(ValueType::INT), data(val) {}
    explicit Value(double val) : type(ValueType::FLOAT), data(val) {}
    explicit Value(const std::string& val) : type(ValueType::STRING), data(val) {}
    
    // 函数类型
    Value(std::shared_ptr<FunctionDecl> func, std::shared_ptr<Environment> closure)
        : type(ValueType::FUNCTION), data(func), closure(closure) {}

    ValueType getType() const { return type; }
    
    bool isNull() const { return type == ValueType::NULL_TYPE; }
    bool isBool() const { return type == ValueType::BOOL; }
    bool isInt() const { return type == ValueType::INT; }
    bool isFloat() const { return type == ValueType::FLOAT; }
    bool isString() const { return type == ValueType::STRING; }
    bool isFunction() const { return type == ValueType::FUNCTION; }
    bool isReturnValue() const { return type == ValueType::RETURN_VALUE; }

    // 获取值
    bool asBool() const { return std::get<bool>(data); }
    int asInt() const { return std::get<int>(data); }
    double asFloat() const { return std::get<double>(data); }
    std::string asString() const { return std::get<std::string>(data); }
    std::shared_ptr<FunctionDecl> asFunction() const { return std::get<std::shared_ptr<FunctionDecl>>(data); }
    std::shared_ptr<Environment> getClosure() const { return closure; }

    // 创建return包装值
    static Value makeReturn(const Value& val) {
        Value ret;
        ret.type = ValueType::RETURN_VALUE;
        ret.returnValue = std::make_shared<Value>(val);
        return ret;
    }
    
    Value unwrapReturn() const {
        if (returnValue) return *returnValue;
        return Value();
    }

    // 类型转换
    std::string toString() const;
    bool isTruthy() const;
    double toNumber() const;

    // 算术运算
    static Value add(const Value& left, const Value& right);
    static Value subtract(const Value& left, const Value& right);
    static Value multiply(const Value& left, const Value& right);
    static Value divide(const Value& left, const Value& right);
    static Value modulo(const Value& left, const Value& right);

    // 比较运算
    static Value equal(const Value& left, const Value& right);
    static Value notEqual(const Value& left, const Value& right);
    static Value lessThan(const Value& left, const Value& right);
    static Value lessEqual(const Value& left, const Value& right);
    static Value greaterThan(const Value& left, const Value& right);
    static Value greaterEqual(const Value& left, const Value& right);

private:
    ValueType type;
    std::variant<
        std::monostate,  // null
        bool,
        int,
        double,
        std::string,
        std::shared_ptr<FunctionDecl>
    > data;
    
    std::shared_ptr<Value> returnValue;  // 用于RETURN_VALUE类型
    std::shared_ptr<Environment> closure;  // 函数闭包环境
};

} // namespace PHPForge

#endif // VALUE_H
