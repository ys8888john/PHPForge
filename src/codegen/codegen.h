#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ast/ASTNode.h"
#include "../ast/program.h"
#include "../ast/functionDecl.h"
#include "../ast/statement.h"
#include "../ast/expression.h"
#include "llvm/IR/IRBuilder.h"
#include <memory>
#include <string>
#include <map>
#include <vector>

namespace llvm {
    class LLVMContext;
    class Module;
    class Function;
    class Value;
    class AllocaInst;
}

namespace PHPForge {

class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();

    void generate(const Program* program);
    void dumpIR() const;
    bool writeIRToFile(const std::string& filename) const;
    int jit(); // JIT execute, consumes module

    const llvm::Module* getModule() const { return module.get(); }

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Variable scope stack (stores alloca pointers)
    std::vector<std::map<std::string, llvm::Value*>> namedValues;

    llvm::Function* currentFunction = nullptr;
    llvm::Function* printfFunc = nullptr;

    // Visit methods
    void visitProgram(const Program* program);
    llvm::Function* visitFunctionDecl(const FunctionDecl* node);
    void visitBlockStmt(const BlockStmt* node);
    void visitReturnStmt(const ReturnStmt* node);
    void visitEchoStmt(const EchoStmt* node);
    void visitExpressionStmt(const ExpressionStmt* node);

    // Expression codegen
    llvm::Value* codegenExpr(const ASTNode* expr);
    llvm::Value* codegenBinaryExpr(const BinaryExpr* expr);
    llvm::Value* codegenCallExpr(const CallExpr* expr);
    llvm::Value* codegenLiteralExpr(const LiteralExpr* expr);
    llvm::Value* codegenVariableExpr(const VariableExpr* expr);

    // Scope helpers
    void pushScope();
    void popScope();
    void setNamedValue(const std::string& name, llvm::Value* value);
    llvm::Value* getNamedValue(const std::string& name) const;

    // LLVM helpers
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func,
                                               const std::string& varName);
    llvm::Function* getPrintfFunction();
    llvm::Value* createGlobalStringPtr(const std::string& str);
};

} // namespace PHPForge

#endif // CODEGEN_H
