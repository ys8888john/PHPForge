#ifndef STRENGTHREDUCTIONPASS_H
#define STRENGTHREDUCTIONPASS_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace PHPForge {

/// 乘法强度消减 Pass：把 x * 2^n 替换为 x << n
/// 例如：x * 2 → x << 1,  x * 4 → x << 2
class StrengthReductionPass : public llvm::PassInfoMixin<StrengthReductionPass> {
public:
    // 每个 Function 级 Pass 必须实现这个 run 方法
    llvm::PreservedAnalyses run(llvm::Function &F,
                                llvm::FunctionAnalysisManager &AM);

    // 返回 false 表示这个 Pass 不是必需的（可被跳过）
    static bool isRequired() { return false; }
};

} // namespace PHPForge

#endif
