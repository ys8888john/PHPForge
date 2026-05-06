#include "codegen.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "StrengthReductionPass.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
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

void CodeGenerator::optimize() {
    if (!module) {
        std::cerr << "Optimize error: module not generated" << std::endl;
        return;
    }

    llvm::PassBuilder pb;

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;

    // Register all analyses
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    // Build function-level pass pipeline: mem2reg + strengthreduction + instcombine + simplifycfg
    llvm::FunctionPassManager fpm;
    fpm.addPass(llvm::PromotePass());              // mem2reg: alloca → SSA
    fpm.addPass(PHPForge::StrengthReductionPass());// 乘法强度消减: x*2^n → x<<n
    fpm.addPass(llvm::InstCombinePass());          // 指令化简
    fpm.addPass(llvm::GVNPass());                  // 全局值编号
    fpm.addPass(llvm::SimplifyCFGPass());          // 简化控制流图

    // Wrap into module pass
    llvm::ModulePassManager mpm;
    mpm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(fpm)));

    mpm.run(*module, mam);

    std::cout << "=== 优化完成 (mem2reg + strengthreduction + instcombine + gvn + simplifycfg) ===" << std::endl;
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

llvm::Type* CodeGenerator::getLLVMType(const std::string& phpType) {
    if (phpType == "int") {
        return llvm::Type::getInt32Ty(*context);
    } else if (phpType == "string") {
        return llvm::Type::getInt8PtrTy(*context);
    } else if (phpType == "bool") {
        return llvm::Type::getInt1Ty(*context);
    }

    return nullptr;
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

void CodeGenerator::setNamedValue(const std::string& name, llvm::AllocaInst* value) {
    if (!namedValues.empty()) {
        namedValues.back()[name] = value;
    }
}

llvm::AllocaInst* CodeGenerator::getNamedValue(const std::string& name) const {
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
                                                         llvm::Type* varType) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());

    if (varType == llvm::Type::getInt32Ty(*context)) {
        return tmpBuilder.CreateAlloca(llvm::Type::getInt32Ty(*context),
                                    nullptr);
    }

    if (varType == llvm::Type::getInt8PtrTy(*context)) {
        return tmpBuilder.CreateAlloca(llvm::Type::getInt8PtrTy(*context),
                                    nullptr);
    }

    if (varType == llvm::Type::getInt1Ty(*context)) {
        return tmpBuilder.CreateAlloca(llvm::Type::getInt1Ty(*context),
                                    nullptr);
    }

    // Default: allocate for the given type
    return tmpBuilder.CreateAlloca(varType, nullptr);
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
            case ASTNodeType::IF_STMT:
                visitIfStmt(static_cast<const IfStmt*>(stmt.get()));
                break;
            case ASTNodeType::FOR_STMT:
                visitForStmt(static_cast<const ForStmt*>(stmt.get()));
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

    std::vector<llvm::Type*> paramTypes;
    for (auto &param : params) {
        paramTypes.push_back(getLLVMType(param->getType()));
    }

    auto* funcRetType = getLLVMType(node->getReturnType());
    auto* funcType = llvm::FunctionType::get(
        funcRetType,
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
        auto* alloca = createEntryBlockAlloca(func, arg.getType());
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

void CodeGenerator::visitStmt(const ASTNode* node) {
    if (!node) return;

    switch (node->getNodeType()) {
        case ASTNodeType::RETURN_STMT:
            visitReturnStmt(static_cast<const ReturnStmt*>(node));
            break;
        case ASTNodeType::ECHO_STMT:
            visitEchoStmt(static_cast<const EchoStmt*>(node));
            break;
        case ASTNodeType::EXPRESSION_STMT:
            visitExpressionStmt(static_cast<const ExpressionStmt*>(node));
            break;
        case ASTNodeType::BLOCK_STMT:
            visitBlockStmt(static_cast<const BlockStmt*>(node));
            break;
        case ASTNodeType::IF_STMT:
            visitIfStmt(static_cast<const IfStmt*>(node));
            break;
        case ASTNodeType::FOR_STMT:
            visitForStmt(static_cast<const ForStmt*>(node));
            break;
        default:
            break;
    }
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
            case ASTNodeType::IF_STMT:
                visitIfStmt(static_cast<const IfStmt*>(s.get()));
                break;
            case ASTNodeType::FOR_STMT:
                visitForStmt(static_cast<const ForStmt*>(s.get()));
                break;
            default:
                break;
        }
    }
}

void CodeGenerator::visitIfStmt(const IfStmt* stmt) {
    llvm::Value* cond = codegenExpr(stmt->getCondition());
    if (!cond) return;

    // Convert cond to i1 if needed
    if (!cond->getType()->isIntegerTy(1)) {
        cond = builder->CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0), "ifcond");
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // Create blocks for then, else (optional), and merge
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", func);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "merge");

    bool hasElse = (stmt->getElseBranch() != nullptr);
    if (!hasElse) {
        // No else branch: elseBB merges directly
        elseBB = mergeBB;
    }

    builder->CreateCondBr(cond, thenBB, elseBB);

    // --- Then block ---
    builder->SetInsertPoint(thenBB);
    if (stmt->getThenBranch()) {
        visitStmt(stmt->getThenBranch());
    }
    // If then block doesn't have a terminator, branch to merge
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }

    // --- Else block ---
    if (hasElse) {
        func->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);
        visitStmt(stmt->getElseBranch());
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
    }

    // --- Merge block ---
    func->getBasicBlockList().push_back(mergeBB);
    builder->SetInsertPoint(mergeBB);
}

void CodeGenerator::visitForStmt(const ForStmt* stmt) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    // Create blocks: cond, body, update, after
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "for.cond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "for.body", func);
    llvm::BasicBlock* updateBB = llvm::BasicBlock::Create(*context, "for.update", func);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "for.after");

    // Emit init
    if (stmt->getInit()) {
        codegenExpr(stmt->getInit());
    }

    // Branch to condition check
    builder->CreateBr(condBB);

    // --- Condition block ---
    builder->SetInsertPoint(condBB);
    if (stmt->getCondition()) {
        llvm::Value* condVal = codegenExpr(stmt->getCondition());
        if (condVal) {
            // Convert to i1 if needed
            if (!condVal->getType()->isIntegerTy(1)) {
                condVal = builder->CreateICmpNE(condVal,
                    llvm::ConstantInt::get(condVal->getType(), 0), "forcond");
            }
            builder->CreateCondBr(condVal, bodyBB, afterBB);
        } else {
            builder->CreateBr(bodyBB);
        }
    } else {
        // No condition = infinite loop (but we always go to body)
        builder->CreateBr(bodyBB);
    }

    // --- Body block ---
    builder->SetInsertPoint(bodyBB);
    if (stmt->getBody()) {
        visitStmt(stmt->getBody());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(updateBB);
    }

    // --- Update block ---
    builder->SetInsertPoint(updateBB);
    if (stmt->getUpdate()) {
        codegenExpr(stmt->getUpdate());
    }
    builder->CreateBr(condBB);

    // --- After block ---
    func->getBasicBlockList().push_back(afterBB);
    builder->SetInsertPoint(afterBB);
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

    auto* printfFunc = getPrintfFunction();

    // String type: printf("%s\n", str)
    if (val->getType()->isPointerTy()) {
        auto* fmtStr = createGlobalStringPtr("%s\n");
        builder->CreateCall(printfFunc, {fmtStr, val});
        return;
    }

    // i1 (bool): extend to i32, then printf("%d\n", val)
    if (val->getType()->isIntegerTy(1)) {
        val = builder->CreateZExt(val, llvm::Type::getInt32Ty(*context), "boolext");
    }

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

    if (expr->getType() == "bool") {
        bool val = expr->getValue() == "true";
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), val, true);
    }

    if (expr->getType() == "string") {
        return createGlobalStringPtr(expr->getValue());
    }

    std::cerr << "Codegen error: unsupported literal type '" << expr->getType()
              << "' with value '" << expr->getValue() << "'" << std::endl;
    return nullptr;
}

llvm::Value* CodeGenerator::codegenVariableExpr(const VariableExpr* expr) {
    llvm::AllocaInst* alloca = getNamedValue(expr->getName());
    if (!alloca) {
        std::cerr << "Codegen error: unknown variable '$" << expr->getName()
                  << "'" << std::endl;
        return nullptr;
    }

    llvm::Type* varType = alloca->getAllocatedType();
    return builder->CreateLoad(varType, alloca, expr->getName());
}

llvm::Value* CodeGenerator::codegenBinaryExpr(const BinaryExpr* expr) {
    const std::string& op = expr->getOp();

    // Assignment: $var = expr
    if (op == "=") {
        auto* lhs = static_cast<const VariableExpr*>(expr->getLeft());
        llvm::Value* rhsVal = codegenExpr(expr->getRight());
        if (!rhsVal) return nullptr;

        llvm::AllocaInst* varSlot = getNamedValue(lhs->getName());
        if (!varSlot) {
            // First assignment — create alloca in function entry block
            varSlot = createEntryBlockAlloca(currentFunction, rhsVal->getType());
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
