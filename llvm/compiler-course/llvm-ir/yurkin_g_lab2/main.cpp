#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct ExamplePass : PassInfoMixin<ExamplePass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    for (BasicBlock &BB : F) {
      for (auto It = BB.begin(); It != BB.end();) {
        Instruction &I = *It++;

        // Проверяем opcode вместо конкретного класса инструкции.
        if (I.getOpcode() == Instruction::FRem) {
          IRBuilder<> B(&I);
          Value *A = I.getOperand(0);
          Value *Bv = I.getOperand(1);

          Value *Div = B.CreateFDiv(A, Bv, "frem.div");
          Value *Mul = B.CreateFMul(Div, Bv, "frem.mul");
          Value *Sub = B.CreateFSub(A, Mul, "frem.sub");

          I.replaceAllUsesWith(Sub);
          I.eraseFromParent();
          Changed = true;
          continue;
        }

        if (I.getOpcode() == Instruction::SRem) {
          IRBuilder<> B(&I);
          Value *A = I.getOperand(0);
          Value *Bv = I.getOperand(1);

          Value *Div = B.CreateSDiv(A, Bv, "srem.sdiv");
          Value *Mul = B.CreateMul(Div, Bv, "srem.mul");
          Value *Sub = B.CreateSub(A, Mul, "srem.sub");

          I.replaceAllUsesWith(Sub);
          I.eraseFromParent();
          Changed = true;
          continue;
        }

        if (I.getOpcode() == Instruction::URem) {
          IRBuilder<> B(&I);
          Value *A = I.getOperand(0);
          Value *Bv = I.getOperand(1);

          Value *Div = B.CreateUDiv(A, Bv, "urem.udiv");
          Value *Mul = B.CreateMul(Div, Bv, "urem.mul");
          Value *Sub = B.CreateSub(A, Mul, "urem.sub");

          I.replaceAllUsesWith(Sub);
          I.eraseFromParent();
          Changed = true;
          continue;
        }
      }
    }

    if (Changed)
      return PreservedAnalyses::none();
    return PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ExamplePass", "0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) -> bool {
                  if (Name == "example") {
                    FPM.addPass(ExamplePass{});
                    return true;
                  }
                  return false;
                });
          }};
}
