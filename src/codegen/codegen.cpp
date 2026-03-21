#include "codegen.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/Orc/EPCDynamicLibrarySearchGenerator.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>

namespace PHPForge {

CodeGenerator::CodeGenerator()
    : context(std::make_unique<llvm::LLVMContext>()),
      module(std::make_unique<llvm::Module>("PHPForge", *context)),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)) {}

CodeGenerator::~CodeGenerator() = default;

void CodeGenerator::generate(const Program* program) {
    visitProgram(program);
}

void CodeGenerator::dumpIR() const {
    module->print(llvm::outs(), nullptr);
}

bool CodeGenerator::writeIRToFile(const std::string& filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC);
    if (EC) {
        std::cerr << "Error opening file '" << filename << "': " << EC.message() << std::endl;
        return false;
    }
    module->print(out, nullptr);
    return true;
}

// ============================================================
// Scope management
// ============================================================

void CodeGenerator::pushScope() {
    namedValues.push_back({});
}

void CodeGenerator::popScope() {
    if (!namedValues.empty()) {
        namedValues.pop_back();
    }
}

void CodeGenerator::setNamedValue(const std::string& name, llvm::Value* value) {
    if (!namedValues.empty()) {
        namedValues.back()[name] = value;
    }
}

llvm::Value* CodeGenerator::getNamedValue(const std::string& name) const {
    for (auto it = namedValues.rbegin(); it != namedValues.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}

// ============================================================
// LLVM helpers
// ============================================================

llvm::AllocaInst* CodeGenerator::createEntryBlockAlloca(llvm::Function* func,
                                                         const std::string& varName) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(llvm::Type::getInt32Ty(*context),
                                    nullptr, varName);
}

llvm::Function* CodeGenerator::getPrintfFunction() {
    if (printfFunc) return printfFunc;

    std::vector<llvm::Type*> printfArgTypes;
    printfArgTypes.push_back(llvm::Type::getInt8PtrTy(*context));

    auto* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        printfArgTypes,
        true // variadic
    );

    printfFunc = llvm::Function::Create(
        printfType,
        llvm::Function::ExternalLinkage,
        "printf",
        module.get()
    );

    return printfFunc;
}

llvm::Value* CodeGenerator::createGlobalStringPtr(const std::string& str) {
    return builder->CreateGlobalStringPtr(str, "str");
}

// ============================================================
// Program & statement visitors
// ============================================================

void CodeGenerator::visitProgram(const Program* program) {
    // Create C main() for top-level PHP code
    auto* mainFunc = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt32Ty(*context), false),
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    auto* entryBlock = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entryBlock);

    // Set current function context for top-level code
    auto* prevFunction = currentFunction;
    currentFunction = mainFunc;
    pushScope();

    for (const auto& stmt : program->getStatements()) {
        switch (stmt->getNodeType()) {
            case ASTNodeType::FUNCTION_DECL: {
                // Save/restore insert point for function body generation
                auto* curBlock = builder->GetInsertBlock();
                auto curPoint = builder->GetInsertPoint();

                visitFunctionDecl(static_cast<const FunctionDecl*>(stmt.get()));

                builder->SetInsertPoint(curBlock, curPoint);
                break;
            }
            case ASTNodeType::DECLARE_STMT:
                break; // skip declare(strict_types=1)
            case ASTNodeType::ECHO_STMT:
                visitEchoStmt(static_cast<const EchoStmt*>(stmt.get()));
                break;
            case ASTNodeType::EXPRESSION_STMT:
                visitExpressionStmt(static_cast<const ExpressionStmt*>(stmt.get()));
                break;
            default:
                break;
        }
    }

    // Return 0 from main
    builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));

    popScope();
    currentFunction = prevFunction;

    // Verify all functions
    for (auto& func : *module) {
        if (!func.isDeclaration()) {
            std::string err;
            llvm::raw_string_ostream errStream(err);
            if (llvm::verifyFunction(func, &errStream)) {
                std::cerr << "LLVM verification error in function '"
                          << func.getName().str() << "': " << err << std::endl;
            }
        }
    }
}

llvm::Function* CodeGenerator::visitFunctionDecl(const FunctionDecl* node) {
    const auto& params = node->getParameters();

    // Build parameter types (all int for now)
    std::vector<llvm::Type*> paramTypes(params.size(),
        llvm::Type::getInt32Ty(*context));

    auto* funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        paramTypes,
        false
    );

    auto* func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        node->getName(),
        module.get()
    );

    // Name the arguments
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(params[idx]->getName());
        idx++;
    }

    // Create entry basic block
    auto* entryBlock = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entryBlock);

    // Save and set current function context
    auto* prevFunction = currentFunction;
    currentFunction = func;
    pushScope();

    // Allocate parameters and store them
    for (auto& arg : func->args()) {
        auto* alloca = createEntryBlockAlloca(func, arg.getName().str());
        builder->CreateStore(&arg, alloca);
        setNamedValue(arg.getName().str(), alloca);
    }

    // Generate function body
    if (auto* body = node->getBody()) {
        visitBlockStmt(body);
    }

    // Ensure every basic block has a terminator
    auto* lastBlock = builder->GetInsertBlock();
    if (lastBlock && !lastBlock->getTerminator()) {
        builder->CreateRet(llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(*context), 0));
    }

    popScope();
    currentFunction = prevFunction;

    return func;
}

void CodeGenerator::visitBlockStmt(const BlockStmt* stmt) {
    for (const auto& s : stmt->getStatements()) {
        // Skip if current block already has a terminator (e.g. after return)
        if (builder->GetInsertBlock()->getTerminator()) {
            break;
        }

        switch (s->getNodeType()) {
            case ASTNodeType::RETURN_STMT:
                visitReturnStmt(static_cast<const ReturnStmt*>(s.get()));
                break;
            case ASTNodeType::ECHO_STMT:
                visitEchoStmt(static_cast<const EchoStmt*>(s.get()));
                break;
            case ASTNodeType::EXPRESSION_STMT:
                visitExpressionStmt(static_cast<const ExpressionStmt*>(s.get()));
                break;
            case ASTNodeType::BLOCK_STMT:
                visitBlockStmt(static_cast<const BlockStmt*>(s.get()));
                break;
            default:
                break;
        }
    }
}

void CodeGenerator::visitReturnStmt(const ReturnStmt* stmt) {
    if (stmt->getExpr()) {
        llvm::Value* val = codegenExpr(stmt->getExpr());
        if (val) {
            builder->CreateRet(val);
        }
    } else {
        builder->CreateRet(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
    }
}

void CodeGenerator::visitEchoStmt(const EchoStmt* stmt) {
    llvm::Value* val = codegenExpr(stmt->getExpr());
    if (!val) return;

    // Extend i1 to i32 if necessary
    if (val->getType()->isIntegerTy(1)) {
        val = builder->CreateZExt(val, llvm::Type::getInt32Ty(*context), "boolext");
    }

    // printf("%d\n", val)
    auto* printfFunc = getPrintfFunction();
    auto* fmtStr = createGlobalStringPtr("%d\n");
    builder->CreateCall(printfFunc, {fmtStr, val});
}

void CodeGenerator::visitExpressionStmt(const ExpressionStmt* stmt) {
    codegenExpr(stmt->getExpr());
}

// ============================================================
// Expression codegen
// ============================================================

llvm::Value* CodeGenerator::codegenExpr(const ASTNode* expr) {
    if (!expr) return nullptr;

    switch (expr->getNodeType()) {
        case ASTNodeType::LITERAL:
            return codegenLiteralExpr(static_cast<const LiteralExpr*>(expr));
        case ASTNodeType::VARIABLE:
            return codegenVariableExpr(static_cast<const VariableExpr*>(expr));
        case ASTNodeType::BINARY_EXPR:
            return codegenBinaryExpr(static_cast<const BinaryExpr*>(expr));
        case ASTNodeType::CALL_EXPR:
            return codegenCallExpr(static_cast<const CallExpr*>(expr));
        default:
            std::cerr << "Codegen error: unsupported expression type ("
                      << static_cast<int>(expr->getNodeType()) << ")" << std::endl;
            return nullptr;
    }
}

llvm::Value* CodeGenerator::codegenLiteralExpr(const LiteralExpr* expr) {
    if (expr->getType() == "int") {
        int64_t val = std::stoll(expr->getValue());
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), val, true);
    }

    std::cerr << "Codegen error: unsupported literal type '" << expr->getType()
              << "' with value '" << expr->getValue() << "'" << std::endl;
    return nullptr;
}

llvm::Value* CodeGenerator::codegenVariableExpr(const VariableExpr* expr) {
    llvm::Value* alloca = getNamedValue(expr->getName());
    if (!alloca) {
        std::cerr << "Codegen error: unknown variable '$" << expr->getName()
                  << "'" << std::endl;
        return nullptr;
    }
    return builder->CreateLoad(llvm::Type::getInt32Ty(*context),
                                alloca, expr->getName());
}

llvm::Value* CodeGenerator::codegenBinaryExpr(const BinaryExpr* expr) {
    const std::string& op = expr->getOp();

    // Assignment: $var = expr
    if (op == "=") {
        auto* lhs = static_cast<const VariableExpr*>(expr->getLeft());
        llvm::Value* rhsVal = codegenExpr(expr->getRight());
        if (!rhsVal) return nullptr;

        llvm::Value* varSlot = getNamedValue(lhs->getName());
        if (!varSlot) {
            // First assignment — create alloca in function entry block
            varSlot = createEntryBlockAlloca(currentFunction, lhs->getName());
            setNamedValue(lhs->getName(), varSlot);
        }

        builder->CreateStore(rhsVal, varSlot);
        return rhsVal;
    }

    // Arithmetic / comparison operators
    llvm::Value* L = codegenExpr(expr->getLeft());
    llvm::Value* R = codegenExpr(expr->getRight());
    if (!L || !R) return nullptr;

    if (op == "+")  return builder->CreateAdd(L, R, "addtmp");
    if (op == "-")  return builder->CreateSub(L, R, "subtmp");
    if (op == "*")  return builder->CreateMul(L, R, "multmp");
    if (op == "/")  return builder->CreateSDiv(L, R, "divtmp");
    if (op == "%")  return builder->CreateSRem(L, R, "modtmp");

    // Comparison operators — result is i1, zero-extend to i32
    llvm::Value* cmp = nullptr;
    if (op == "<")  cmp = builder->CreateICmpSLT(L, R, "cmptmp");
    if (op == ">")  cmp = builder->CreateICmpSGT(L, R, "cmptmp");
    if (op == "<=") cmp = builder->CreateICmpSLE(L, R, "cmptmp");
    if (op == ">=") cmp = builder->CreateICmpSGE(L, R, "cmptmp");
    if (op == "==") cmp = builder->CreateICmpEQ(L, R, "cmptmp");
    if (op == "!=") cmp = builder->CreateICmpNE(L, R, "cmptmp");

    if (cmp) {
        return builder->CreateZExt(cmp, llvm::Type::getInt32Ty(*context), "zexttmp");
    }

    std::cerr << "Codegen error: unsupported binary operator '" << op << "'" << std::endl;
    return nullptr;
}

llvm::Value* CodeGenerator::codegenCallExpr(const CallExpr* expr) {
    auto* callee = static_cast<const IdentifierExpr*>(expr->getCallee());
    const std::string& funcName = callee->getName();

    llvm::Function* calleeFunc = module->getFunction(funcName);
    if (!calleeFunc) {
        std::cerr << "Codegen error: call to unknown function '"
                  << funcName << "'" << std::endl;
        return nullptr;
    }

    std::vector<llvm::Value*> args;
    for (const auto& arg : expr->getArguments()) {
        llvm::Value* argVal = codegenExpr(arg.get());
        if (!argVal) return nullptr;
        args.push_back(argVal);
    }

    return builder->CreateCall(calleeFunc, args, "calltmp");
}

// ============================================================
// JIT execution
// ============================================================

int CodeGenerator::jit() {
    using namespace llvm;
    using namespace llvm::orc;

    if (!module) {
        std::cerr << "JIT error: module already consumed or not generated" << std::endl;
        return 1;
    }

    // Initialize native target (required for JIT)
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // Create LLJIT instance
    auto JITExpected = LLJITBuilder().create();
    if (!JITExpected) {
        logAllUnhandledErrors(JITExpected.takeError(), llvm::errs());
        return 1;
    }
    auto& JIT = *JITExpected;

    // Allow JIT to resolve host process symbols (e.g. printf, puts)
    auto DLSG = DynamicLibrarySearchGenerator::GetForCurrentProcess(
        JIT->getDataLayout().getGlobalPrefix());
    if (!DLSG) {
        logAllUnhandledErrors(DLSG.takeError(), llvm::errs());
        return 1;
    }
    JIT->getMainJITDylib().addGenerator(std::move(*DLSG));

    // Move module into ThreadSafeModule (consumes module & context)
    auto TSM = ThreadSafeModule(std::move(module), std::move(context));

    // Add IR module to JIT
    if (auto Err = JIT->addIRModule(std::move(TSM))) {
        logAllUnhandledErrors(std::move(Err), llvm::errs());
        return 1;
    }

    // Look up the generated "main" function
    auto MainAddr = JIT->lookup("main");
    if (!MainAddr) {
        logAllUnhandledErrors(MainAddr.takeError(), llvm::errs());
        return 1;
    }

    // Cast to function pointer and execute
    auto* MainFn = MainAddr->toPtr<int()>();
    return MainFn();
}

} // namespace PHPForge
