#include "StrengthReductionPass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

namespace PHPForge {

llvm::PreservedAnalyses StrengthReductionPass::run(llvm::Function &F,
                                                    llvm::FunctionAnalysisManager &AM) {
    llvm::errs() << "=== [StrengthReduction] Pass BEFORE: " << F.getName() << " ===\n";
    F.print(llvm::errs());

    bool changed = false;
    // 收集需要替换的指令（不能在遍历时直接删除）
    std::vector<std::pair<llvm::BinaryOperator*, llvm::Value*>> replacements;

    for (auto &BB : F) {
        for (auto &I : BB) {
            // 只关注乘法指令
            auto *mul = llvm::dyn_cast<llvm::BinaryOperator>(&I);
            if (!mul || mul->getOpcode() != llvm::Instruction::Mul)
                continue;

            // 检查是否有一个操作数是 2 的幂次常量
            for (unsigned i = 0; i < 2; ++i) {
                auto *constVal = llvm::dyn_cast<llvm::ConstantInt>(mul->getOperand(i));
                if (!constVal) continue;

                uint64_t coeff = constVal->getZExtValue();
                if (coeff < 2) continue;

                // 检查是否是 2 的幂：x & (x-1) == 0
                if ((coeff & (coeff - 1)) != 0) continue;

                // 计算移位位数
                unsigned shiftAmount = llvm::APInt(64, coeff).exactLogBase2();

                // 构造左移指令
                llvm::IRBuilder<> builder(mul);
                auto *otherOp = mul->getOperand(1 - i);
                auto *shift = builder.CreateShl(otherOp, shiftAmount,
                                                 mul->getName() + ".shift");

                replacements.push_back({mul, shift});
                changed = true;
                break; // 这个乘法已处理，跳出操作数循环
            }
        }
    }

    // 执行替换
    for (auto &[oldInst, newVal] : replacements) {
        oldInst->replaceAllUsesWith(newVal);
        oldInst->eraseFromParent();
    }

    if (changed) {
        llvm::errs() << "=== [StrengthReduction] Pass AFTER: " << F.getName() << " ===\n";
        F.print(llvm::errs());
    }

    // 如果做了修改，其他分析可能失效；否则全部保留
    return changed ? llvm::PreservedAnalyses::none()
                   : llvm::PreservedAnalyses::all();
}

} // namespace PHPForge
