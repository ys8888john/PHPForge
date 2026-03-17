#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "value.h"
#include "environment.h"
#include "../ast/ASTNode.h"
#include "../ast/program.h"
#include "../ast/statement.h"
#include "../ast/expression.h"
#include "../ast/functionDecl.h"
#include <memory>
#include <vector>

namespace PHPForge {

class Interpreter {
public:
    Interpreter();
    
    // 解释执行程序
    void interpret(Program* program);
    
    // 获取最后结果
    Value getLastValue() const { return lastValue; }

private:
    // 执行语句
    Value execute(ASTNode* stmt);
    
    // 求值表达式
    Value evaluate(ASTNode* expr);
    
    // 各种语句的执行
    Value executeProgram(Program* program);
    Value executeBlockStmt(BlockStmt* stmt, std::shared_ptr<Environment> env);
    Value executeExpressionStmt(ExpressionStmt* stmt);
    Value executeReturnStmt(ReturnStmt* stmt);
    Value executeEchoStmt(EchoStmt* stmt);
    Value executeDeclareStmt(DeclareStmt* stmt);
    Value executeVariableDecl(ASTNode* stmt);  // 处理变量声明
    
    // 各种表达式的求值
    Value evaluateBinaryExpr(BinaryExpr* expr);
    Value evaluateCallExpr(CallExpr* expr);
    Value evaluateLiteralExpr(LiteralExpr* expr);
    Value evaluateVariableExpr(VariableExpr* expr);
    Value evaluateIdentifierExpr(IdentifierExpr* expr);
    
    // 函数调用
    Value callFunction(const Value& callee, const std::vector<Value>& arguments);
    
    // 检查操作数类型
    void checkNumberOperands(const std::string& op, const Value& left, const Value& right);
    
    std::shared_ptr<Environment> environment;  // 当前环境
    Value lastValue;  // 最后一个表达式的值
};

} // namespace PHPForge

#endif // INTERPRETER_H
