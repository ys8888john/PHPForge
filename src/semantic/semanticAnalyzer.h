#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "scope.h"
#include "symbol.h"
#include "../ast/ASTNode.h"
#include "../ast/program.h"
#include "../ast/functionDecl.h"
#include "../ast/statement.h"
#include "../ast/expression.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace PHPForge {

// 语义错误
class SemanticError : public std::runtime_error {
public:
    SemanticError(const std::string& message, int line, int column)
        : std::runtime_error(message + " at line " + std::to_string(line) + 
                          ", column " + std::to_string(column)),
          line(line), column(column) {}
    
    int getLine() const { return line; }
    int getColumn() const { return column; }

private:
    int line;
    int column;
};

// 语义分析器
class SemanticAnalyzer {
public:
    SemanticAnalyzer() : symbolTable(std::make_unique<SymbolTable>()) {}
    
    // 分析整个程序
    void analyze(const Program* program);
    
    // 访问方法
    void visit(const Program* program);
    void visit(const FunctionDecl* function);
    void visit(const Parameter* parameter);
    void visit(const ReturnStmt* stmt);
    void visit(const EchoStmt* stmt);
    void visit(const ExpressionStmt* stmt);
    void visit(const BlockStmt* stmt);
    void visit(const DeclareStmt* stmt);
    void visit(const BinaryExpr* expr);
    void visit(const CallExpr* expr);
    void visit(const LiteralExpr* expr);
    void visit(const VariableExpr* expr);
    void visit(const IdentifierExpr* expr);
    
    // 符号表访问
    SymbolTable* getSymbolTable() const { return symbolTable.get(); }
    
    // 错误信息
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }
    
    // 打印符号表
    void dumpSymbolTable() const;

private:
    std::unique_ptr<SymbolTable> symbolTable;
    std::vector<std::string> errors;
    std::string currentFunction;
    
    // 辅助方法
    void reportError(const std::string& message, int line, int column);
    std::string checkExpressionType(const ASTNode* expr);
    bool isTypeCompatible(const std::string& type1, const std::string& type2);
    std::string getBinaryOpResultType(const std::string& op, 
                                     const std::string& leftType, 
                                     const std::string& rightType);
};

} // namespace PHPForge

#endif // SEMANTIC_ANALYZER_H
