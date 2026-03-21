#include "semanticAnalyzer.h"
#include "../lexer/tokenType.h"
#include <iostream>
#include <sstream>

namespace PHPForge {

void SemanticAnalyzer::analyze(const Program* program) {
    try {
        visit(program);
    } catch (const SemanticError& e) {
        errors.push_back(e.what());
    }
}

void SemanticAnalyzer::visit(const Program* program) {
    // 遍历所有声明和语句
    for (const auto& stmt : program->getStatements()) {
        switch (stmt->getNodeType()) {
            case ASTNodeType::FUNCTION_DECL:
                visit(static_cast<const FunctionDecl*>(stmt.get()));
                break;
            case ASTNodeType::DECLARE_STMT:
                visit(static_cast<const DeclareStmt*>(stmt.get()));
                break;
            case ASTNodeType::EXPRESSION_STMT:
                visit(static_cast<const ExpressionStmt*>(stmt.get()));
                break;
            case ASTNodeType::ECHO_STMT:
                visit(static_cast<const EchoStmt*>(stmt.get()));
                break;
            default:
                break;
        }
    }
}

void SemanticAnalyzer::visit(const FunctionDecl* function) {
    const std::string& funcName = function->getName();
    
    // 检查函数是否已定义
    if (symbolTable->lookup(funcName)) {
        reportError("Function '" + funcName + "' is already defined",
                   function->getLine(), function->getColumn());
        return;
    }
    
    // 创建函数符号
    auto funcSymbol = std::make_shared<FunctionSymbol>(
        funcName, function->getReturnType()
    );
    
    // 处理参数
    for (const auto& param : function->getParameters()) {
        const auto* paramNode = param.get();
        funcSymbol->addParameter(paramNode->getName(), paramNode->getType());
    }
    
    // 添加到符号表
    symbolTable->addSymbol(funcSymbol);
    
    // 进入函数作用域
    symbolTable->enterScope("function_" + funcName);
    currentFunction = funcName;
    
    // 添加参数到当前作用域
    for (const auto& param : function->getParameters()) {
        const auto* paramNode = param.get();
        auto paramSymbol = std::make_shared<ParameterSymbol>(
            paramNode->getName(), paramNode->getType()
        );
        if (!symbolTable->addSymbol(paramSymbol)) {
            reportError("Parameter '" + paramNode->getName() +
                      "' is already defined",
                      paramNode->getLine(), paramNode->getColumn());
        }
    }
    
    // 访问函数体
    if (auto body = function->getBody()) {
        visit(body);
    }
    
    // 退出函数作用域
    symbolTable->exitScope();
    currentFunction.clear();
}

void SemanticAnalyzer::visit(const Parameter* parameter) {
    // 参数已在函数声明中处理
}

void SemanticAnalyzer::visit(const ReturnStmt* stmt) {
    if (!currentFunction.empty()) {
        auto funcSymbol = symbolTable->lookup(currentFunction);
        if (funcSymbol && funcSymbol->getKind() == SymbolKind::FUNCTION) {
            auto functionSymbol = std::static_pointer_cast<FunctionSymbol>(funcSymbol);
            const std::string& expectedType = functionSymbol->getType();
            
            if (stmt->getExpr()) {
                std::string exprType = checkExpressionType(stmt->getExpr());
                if (!isTypeCompatible(expectedType, exprType)) {
                    reportError("Return type mismatch: expected '" + expectedType + 
                              "' but got '" + exprType + "'",
                              stmt->getLine(), stmt->getColumn());
                }
            }
        }
    }
}

void SemanticAnalyzer::visit(const EchoStmt* stmt) {
    if (stmt->getExpr()) {
        checkExpressionType(stmt->getExpr());
    }
}

void SemanticAnalyzer::visit(const ExpressionStmt* stmt) {
    if (stmt->getExpr()) {
        checkExpressionType(stmt->getExpr());
    }
}

void SemanticAnalyzer::visit(const BlockStmt* stmt) {
    symbolTable->enterScope("block");
    
    for (const auto& s : stmt->getStatements()) {
        switch (s->getNodeType()) {
            case ASTNodeType::RETURN_STMT:
                visit(static_cast<const ReturnStmt*>(s.get()));
                break;
            case ASTNodeType::ECHO_STMT:
                visit(static_cast<const EchoStmt*>(s.get()));
                break;
            case ASTNodeType::EXPRESSION_STMT:
                visit(static_cast<const ExpressionStmt*>(s.get()));
                break;
            case ASTNodeType::BLOCK_STMT:
                visit(static_cast<const BlockStmt*>(s.get()));
                break;
            default:
                break;
        }
    }
    
    symbolTable->exitScope();
}

void SemanticAnalyzer::visit(const DeclareStmt* stmt) {
    // declare 语句用于配置，不影响语义分析
    // 可以记录 strict_types 等配置信息
}

void SemanticAnalyzer::visit(const BinaryExpr* expr) {
    std::string leftType = checkExpressionType(expr->getLeft());
    std::string rightType = checkExpressionType(expr->getRight());
    
    const std::string& op = expr->getOp();
    std::string resultType = getBinaryOpResultType(op, leftType, rightType);
    
    if (resultType.empty()) {
        reportError("Binary operator '" + op + "' cannot be applied to types '" + 
                  leftType + "' and '" + rightType + "'",
                  expr->getLine(), expr->getColumn());
    }
}

void SemanticAnalyzer::visit(const CallExpr* expr) {
    const IdentifierExpr* callee = static_cast<const IdentifierExpr*>(expr->getCallee());
    const std::string& funcName = callee->getName();
    
    auto funcSymbol = symbolTable->lookup(funcName);
    if (!funcSymbol) {
        reportError("Function '" + funcName + "' is not defined",
                  expr->getLine(), expr->getColumn());
        return;
    }
    
    if (funcSymbol->getKind() != SymbolKind::FUNCTION) {
        reportError("'" + funcName + "' is not a function",
                  expr->getLine(), expr->getColumn());
        return;
    }
    
    auto functionSymbol = std::static_pointer_cast<FunctionSymbol>(funcSymbol);
    const auto& paramTypes = functionSymbol->getParameterTypes();
    const auto& args = expr->getArguments();
    
    if (args.size() != paramTypes.size()) {
        reportError("Function '" + funcName + "' expects " + 
                  std::to_string(paramTypes.size()) + " arguments but got " + 
                  std::to_string(args.size()),
                  expr->getLine(), expr->getColumn());
        return;
    }
    
    // 检查参数类型
    for (size_t i = 0; i < args.size(); ++i) {
        std::string argType = checkExpressionType(args[i].get());
        if (!isTypeCompatible(paramTypes[i], argType)) {
            reportError("Argument " + std::to_string(i + 1) + " type mismatch: " +
                      "expected '" + paramTypes[i] + "' but got '" + argType + "'",
                      expr->getLine(), expr->getColumn());
        }
    }
}

void SemanticAnalyzer::visit(const LiteralExpr* expr) {
    // 字面量类型检查
}

void SemanticAnalyzer::visit(const VariableExpr* expr) {
    const std::string& varName = expr->getName();
    
    auto symbol = symbolTable->lookup(varName);
    if (!symbol) {
        reportError("Variable '" + varName + "' is not defined",
                  expr->getLine(), expr->getColumn());
    }
}

void SemanticAnalyzer::visit(const IdentifierExpr* expr) {
    // 标识符已在 CallExpr 中处理
}

std::string SemanticAnalyzer::checkExpressionType(const ASTNode* expr) {
    if (!expr) return "void";
    
    switch (expr->getNodeType()) {
        case ASTNodeType::LITERAL: {
            auto literal = static_cast<const LiteralExpr*>(expr);
            return literal->getType();
        }
        case ASTNodeType::VARIABLE: {
            auto variable = static_cast<const VariableExpr*>(expr);
            auto symbol = symbolTable->lookup(variable->getName());
            if (symbol) {
                return symbol->getType();
            }
            return "unknown";
        }
        case ASTNodeType::BINARY_EXPR: {
            auto binary = static_cast<const BinaryExpr*>(expr);
            std::string leftType = checkExpressionType(binary->getLeft());
            std::string rightType = checkExpressionType(binary->getRight());
            return getBinaryOpResultType(binary->getOp(), leftType, rightType);
        }
        case ASTNodeType::CALL_EXPR: {
            auto call = static_cast<const CallExpr*>(expr);
            auto callee = static_cast<const IdentifierExpr*>(call->getCallee());
            auto symbol = symbolTable->lookup(callee->getName());
            if (symbol && symbol->getKind() == SymbolKind::FUNCTION) {
                auto funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol);
                return funcSymbol->getType();
            }
            return "unknown";
        }
        case ASTNodeType::IDENTIFIER: {
            auto identifier = static_cast<const IdentifierExpr*>(expr);
            auto symbol = symbolTable->lookup(identifier->getName());
            if (symbol) {
                return symbol->getType();
            }
            return "unknown";
        }
        default:
            return "unknown";
    }
}

bool SemanticAnalyzer::isTypeCompatible(const std::string& type1, const std::string& type2) {
    // 简化：只支持 int 类型
    if (type1 == type2) return true;
    
    // PHP 是弱类型语言，允许一定程度的类型转换
    if (type1 == "int" && type2 == "int") return true;
    if (type1 == "int" && type2 == "unknown") return true;
    if (type1 == "unknown" && type2 == "int") return true;
    
    return false;
}

std::string SemanticAnalyzer::getBinaryOpResultType(const std::string& op,
                                                   const std::string& leftType,
                                                   const std::string& rightType) {
    // 简化：只支持加法运算
    if (op == "+" && leftType == "int" && rightType == "int") {
        return "int";
    }
    
    if (leftType == "unknown" || rightType == "unknown") {
        return "unknown";
    }
    
    return "";
}

void SemanticAnalyzer::reportError(const std::string& message, int line, int column) {
    std::string error = "Semantic Error: " + message + " at line " + 
                       std::to_string(line) + ", column " + std::to_string(column);
    errors.push_back(error);
    std::cerr << error << std::endl;
}

void SemanticAnalyzer::dumpSymbolTable() const {
    std::cout << "\n=== Symbol Table ===\n";
    symbolTable->dump();
    std::cout << "====================\n\n";
}

} // namespace PHPForge
