#include "interpreter.h"
#include <iostream>
#include <stdexcept>

namespace PHPForge {

Interpreter::Interpreter() {
    environment = std::make_shared<Environment>();
}

void Interpreter::interpret(Program* program) {
    try {
        executeProgram(program);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        throw;
    }
}

Value Interpreter::execute(ASTNode* stmt) {
    if (!stmt) return Value();
    
    switch (stmt->getNodeType()) {
        case ASTNodeType::PROGRAM:
            return executeProgram(static_cast<Program*>(stmt));
        case ASTNodeType::BLOCK_STMT:
            return executeBlockStmt(static_cast<BlockStmt*>(stmt), std::make_shared<Environment>(environment));
        case ASTNodeType::EXPRESSION_STMT:
            return executeExpressionStmt(static_cast<ExpressionStmt*>(stmt));
        case ASTNodeType::RETURN_STMT:
            return executeReturnStmt(static_cast<ReturnStmt*>(stmt));
        case ASTNodeType::ECHO_STMT:
            return executeEchoStmt(static_cast<EchoStmt*>(stmt));
        case ASTNodeType::DECLARE_STMT:
            return executeDeclareStmt(static_cast<DeclareStmt*>(stmt));
        case ASTNodeType::FUNCTION_DECL:
            // 函数声明在全局环境注册
            {
                auto func = static_cast<FunctionDecl*>(stmt);
                // 使用自定义删除器的shared_ptr，避免删除原始AST节点
                std::shared_ptr<FunctionDecl> funcPtr(
                    func, 
                    [](FunctionDecl*) { /* 不删除，AST由parser管理 */ }
                );
                environment->define(func->getName(), Value(funcPtr, environment));
                return Value();
            }
        case ASTNodeType::VARIABLE_DECL:
            return executeVariableDecl(stmt);
        default:
            // 尝试作为表达式求值
            return evaluate(stmt);
    }
}

Value Interpreter::evaluate(ASTNode* expr) {
    if (!expr) return Value();
    
    switch (expr->getNodeType()) {
        case ASTNodeType::BINARY_EXPR:
            return evaluateBinaryExpr(static_cast<BinaryExpr*>(expr));
        case ASTNodeType::CALL_EXPR:
            return evaluateCallExpr(static_cast<CallExpr*>(expr));
        case ASTNodeType::LITERAL:
            return evaluateLiteralExpr(static_cast<LiteralExpr*>(expr));
        case ASTNodeType::VARIABLE:
            return evaluateVariableExpr(static_cast<VariableExpr*>(expr));
        case ASTNodeType::IDENTIFIER:
            return evaluateIdentifierExpr(static_cast<IdentifierExpr*>(expr));
        default:
            throw std::runtime_error("Unknown expression type at line " + 
                std::to_string(expr->getLine()));
    }
}

Value Interpreter::executeProgram(Program* program) {
    Value result;
    for (const auto& stmt : program->getStatements()) {
        result = execute(stmt.get());
        // 如果是return语句，解包返回值
        if (result.isReturnValue()) {
            return result.unwrapReturn();
        }
    }
    return result;
}

Value Interpreter::executeBlockStmt(BlockStmt* stmt, std::shared_ptr<Environment> env) {
    auto previous = environment;
    environment = env;
    
    Value result;
    try {
        for (const auto& s : stmt->getStatements()) {
            result = execute(s.get());
            // 如果遇到return，向上传递
            if (result.isReturnValue()) {
                break;
            }
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    
    environment = previous;
    return result;
}

Value Interpreter::executeExpressionStmt(ExpressionStmt* stmt) {
    lastValue = evaluate(stmt->getExpr());
    return lastValue;
}

Value Interpreter::executeReturnStmt(ReturnStmt* stmt) {
    Value value;
    if (stmt->getExpr()) {
        value = evaluate(stmt->getExpr());
    }
    return Value::makeReturn(value);
}

Value Interpreter::executeEchoStmt(EchoStmt* stmt) {
    Value value = evaluate(stmt->getExpr());
    std::cout << value.toString() << std::endl;
    return Value();
}

Value Interpreter::executeDeclareStmt(DeclareStmt* stmt) {
    // declare 语句目前只是记录，不影响执行
    // 例如 declare(strict_types=1);
    return Value();
}

Value Interpreter::executeVariableDecl(ASTNode* stmt) {
    // 变量声明在解析阶段已经处理，这里直接返回
    // 实际逻辑在赋值表达式中处理
    return Value();
}

Value Interpreter::evaluateBinaryExpr(BinaryExpr* expr) {
    const std::string& op = expr->getOp();
    
    // 赋值操作特殊处理：先求值右边，再处理左边
    if (op == "=") {
        // 先求值右边表达式
        Value right = evaluate(const_cast<ASTNode*>(expr->getRight()));
        
        // 左边必须是变量，不需要求值，只需要获取变量名
        auto leftNode = const_cast<ASTNode*>(expr->getLeft());
        if (leftNode->getNodeType() == ASTNodeType::VARIABLE) {
            auto var = static_cast<VariableExpr*>(leftNode);
            // 如果变量不存在，先定义；否则赋值
            if (!environment->contains(var->getName())) {
                environment->define(var->getName(), right);
            } else {
                environment->assign(var->getName(), right);
            }
            return right;
        }
        throw std::runtime_error("Invalid assignment target at line " + 
            std::to_string(expr->getLine()));
    }
    
    // 其他二元操作符：先求值左右两边
    Value left = evaluate(const_cast<ASTNode*>(expr->getLeft()));
    Value right = evaluate(const_cast<ASTNode*>(expr->getRight()));
    
    if (op == "+") return Value::add(left, right);
    if (op == "-") return Value::subtract(left, right);
    if (op == "*") return Value::multiply(left, right);
    if (op == "/") return Value::divide(left, right);
    if (op == "%") return Value::modulo(left, right);
    if (op == "==") return Value::equal(left, right);
    if (op == "!=") return Value::notEqual(left, right);
    if (op == "<") return Value::lessThan(left, right);
    if (op == "<=") return Value::lessEqual(left, right);
    if (op == ">") return Value::greaterThan(left, right);
    if (op == ">=") return Value::greaterEqual(left, right);
    if (op == ".") return Value::add(left, right);  // 字符串连接
    
    throw std::runtime_error("Unknown operator: " + op + " at line " + 
        std::to_string(expr->getLine()));
}

Value Interpreter::evaluateCallExpr(CallExpr* expr) {
    Value callee = evaluate(const_cast<ASTNode*>(expr->getCallee()));
    
    // 求值参数
    std::vector<Value> arguments;
    for (const auto& arg : expr->getArgs()) {
        arguments.push_back(evaluate(arg.get()));
    }
    
    return callFunction(callee, arguments);
}

Value Interpreter::evaluateLiteralExpr(LiteralExpr* expr) {
    const std::string& type = expr->getType();
    const std::string& value = expr->getValue();
    
    if (type == "int") {
        return Value(std::stoi(value));
    } else if (type == "float" || type == "double") {
        return Value(std::stod(value));
    } else if (type == "string") {
        return Value(value);
    } else if (type == "bool") {
        return Value(value == "true");
    } else if (type == "null") {
        return Value();
    }
    
    throw std::runtime_error("Unknown literal type: " + type);
}

Value Interpreter::evaluateVariableExpr(VariableExpr* expr) {
    return environment->get(expr->getName());
}

Value Interpreter::evaluateIdentifierExpr(IdentifierExpr* expr) {
    // 标识符作为变量名查找
    return environment->get(expr->getName());
}

Value Interpreter::callFunction(const Value& callee, const std::vector<Value>& arguments) {
    if (!callee.isFunction()) {
        throw std::runtime_error("Can only call functions");
    }
    
    // 获取函数声明
    auto funcDecl = callee.asFunction();
    auto closure = callee.getClosure();
    
    // 创建函数执行环境
    auto funcEnv = std::make_shared<Environment>(closure);
    
    // 绑定参数
    const auto& params = funcDecl->getParams();
    if (arguments.size() != params.size()) {
        throw std::runtime_error("Function " + funcDecl->getName() + 
            " expects " + std::to_string(params.size()) + 
            " arguments but got " + std::to_string(arguments.size()));
    }
    
    for (size_t i = 0; i < params.size(); i++) {
        funcEnv->define(params[i]->getName(), arguments[i]);
    }
    
    // 执行函数体
    Value result = executeBlockStmt(const_cast<BlockStmt*>(funcDecl->getBody().get()), funcEnv);
    
    // 解包return值
    if (result.isReturnValue()) {
        return result.unwrapReturn();
    }
    
    return Value();  // 无返回值，返回null
}

void Interpreter::checkNumberOperands(const std::string& op, const Value& left, const Value& right) {
    if (!left.isInt() && !left.isFloat()) {
        throw std::runtime_error("Left operand must be a number for operator: " + op);
    }
    if (!right.isInt() && !right.isFloat()) {
        throw std::runtime_error("Right operand must be a number for operator: " + op);
    }
}

} // namespace PHPForge
