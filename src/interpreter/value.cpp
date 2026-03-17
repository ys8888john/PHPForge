#include "value.h"
#include <stdexcept>
#include <sstream>

namespace PHPForge {

std::string Value::toString() const {
    switch (type) {
        case ValueType::NULL_TYPE:
            return "null";
        case ValueType::BOOL:
            return asBool() ? "true" : "false";
        case ValueType::INT:
            return std::to_string(asInt());
        case ValueType::FLOAT:
            return std::to_string(asFloat());
        case ValueType::STRING:
            return asString();
        case ValueType::FUNCTION:
            return "<function>";
        case ValueType::RETURN_VALUE:
            return returnValue ? returnValue->toString() : "null";
        default:
            return "unknown";
    }
}

bool Value::isTruthy() const {
    switch (type) {
        case ValueType::NULL_TYPE:
            return false;
        case ValueType::BOOL:
            return asBool();
        case ValueType::INT:
            return asInt() != 0;
        case ValueType::FLOAT:
            return asFloat() != 0.0;
        case ValueType::STRING:
            return !asString().empty();
        case ValueType::RETURN_VALUE:
            return returnValue ? returnValue->isTruthy() : false;
        default:
            return true;
    }
}

double Value::toNumber() const {
    switch (type) {
        case ValueType::INT:
            return static_cast<double>(asInt());
        case ValueType::FLOAT:
            return asFloat();
        case ValueType::STRING:
            try {
                return std::stod(asString());
            } catch (...) {
                return 0.0;
            }
        case ValueType::BOOL:
            return asBool() ? 1.0 : 0.0;
        case ValueType::RETURN_VALUE:
            return returnValue ? returnValue->toNumber() : 0.0;
        default:
            return 0.0;
    }
}

Value Value::add(const Value& left, const Value& right) {
    // 字符串拼接
    if (left.isString() || right.isString()) {
        return Value(left.toString() + right.toString());
    }
    
    // 数值相加
    double result = left.toNumber() + right.toNumber();
    
    // 如果都是整数，返回整数
    if (left.isInt() && right.isInt()) {
        return Value(static_cast<int>(result));
    }
    return Value(result);
}

Value Value::subtract(const Value& left, const Value& right) {
    double result = left.toNumber() - right.toNumber();
    if (left.isInt() && right.isInt()) {
        return Value(static_cast<int>(result));
    }
    return Value(result);
}

Value Value::multiply(const Value& left, const Value& right) {
    double result = left.toNumber() * right.toNumber();
    if (left.isInt() && right.isInt()) {
        return Value(static_cast<int>(result));
    }
    return Value(result);
}

Value Value::divide(const Value& left, const Value& right) {
    double divisor = right.toNumber();
    if (divisor == 0.0) {
        throw std::runtime_error("Division by zero");
    }
    double result = left.toNumber() / divisor;
    if (left.isInt() && right.isInt()) {
        return Value(static_cast<int>(result));
    }
    return Value(result);
}

Value Value::modulo(const Value& left, const Value& right) {
    if (!left.isInt() || !right.isInt()) {
        throw std::runtime_error("Operands must be integers for modulo operation");
    }
    int divisor = right.asInt();
    if (divisor == 0) {
        throw std::runtime_error("Division by zero");
    }
    return Value(left.asInt() % divisor);
}

Value Value::equal(const Value& left, const Value& right) {
    if (left.getType() != right.getType()) {
        // 类型不同，尝试数值比较
        return Value(left.toNumber() == right.toNumber());
    }
    
    switch (left.getType()) {
        case ValueType::NULL_TYPE:
            return Value(true);
        case ValueType::BOOL:
            return Value(left.asBool() == right.asBool());
        case ValueType::INT:
            return Value(left.asInt() == right.asInt());
        case ValueType::FLOAT:
            return Value(left.asFloat() == right.asFloat());
        case ValueType::STRING:
            return Value(left.asString() == right.asString());
        default:
            return Value(false);
    }
}

Value Value::notEqual(const Value& left, const Value& right) {
    return Value(!equal(left, right).asBool());
}

Value Value::lessThan(const Value& left, const Value& right) {
    return Value(left.toNumber() < right.toNumber());
}

Value Value::lessEqual(const Value& left, const Value& right) {
    return Value(left.toNumber() <= right.toNumber());
}

Value Value::greaterThan(const Value& left, const Value& right) {
    return Value(left.toNumber() > right.toNumber());
}

Value Value::greaterEqual(const Value& left, const Value& right) {
    return Value(left.toNumber() >= right.toNumber());
}

} // namespace PHPForge
