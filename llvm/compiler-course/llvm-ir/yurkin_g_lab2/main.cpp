
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace llvm;

namespace {
struct DecomposeRemPass : PassInfoMixin<DecomposeRemPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;
    errs() << "DecomposeRemPass: running on function " << F.getName() << "\n";
    std::vector<Instruction *> Worklist;

    // Собираем все инструкции frem/srem/urem заранее
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        unsigned Op = I.getOpcode();
        if (Op == Instruction::FRem || Op == Instruction::SRem ||
            Op == Instruction::URem) {
          Worklist.push_back(&I);
        }
      }
    }

    errs() << "DecomposeRemPass: found " << Worklist.size()
           << " remainder instructions in " << F.getName() << "\n";

    for (Instruction *I : Worklist) {
      if (!I || !I->getParent())
        continue;

      Value *A = I->getOperand(0);
      Value *Bv = I->getOperand(1);

      // Safety: если операнды отсутствуют — пропускаем
      if (!A || !Bv) {
        errs() << "DecomposeRemPass: skipping malformed rem instruction\n";
        continue;
      }

      IRBuilder<> Builder(I);

      // Для FP: сохранить fast-math флаги, если есть
      FastMathFlags FMF;
      if (auto *FPO = dyn_cast<FPMathOperator>(I))
        FMF = FPO->getFastMathFlags();
      Builder.setFastMathFlags(FMF);

      Value *Div = nullptr;
      Value *Mul = nullptr;
      Value *Sub = nullptr;

      switch (I->getOpcode()) {
      case Instruction::FRem: {
        errs() << "DecomposeRemPass: replacing frem in " << F.getName() << "\n";
        Div = Builder.CreateFDiv(A, Bv, "frem.div");
        Mul = Builder.CreateFMul(Div, Bv, "frem.mul");
        Sub = Builder.CreateFSub(A, Mul, "frem.sub");
        break;
      }
      case Instruction::SRem: {
        errs() << "DecomposeRemPass: replacing srem in " << F.getName() << "\n";
        Div = Builder.CreateSDiv(A, Bv, "srem.sdiv");
        Mul = Builder.CreateMul(Div, Bv, "srem.mul");
        Sub = Builder.CreateSub(A, Mul, "srem.sub");
        break;
      }
      case Instruction::URem: {
        errs() << "DecomposeRemPass: replacing urem in " << F.getName() << "\n";
        Div = Builder.CreateUDiv(A, Bv, "urem.udiv");
        Mul = Builder.CreateMul(Div, Bv, "urem.mul");
        Sub = Builder.CreateSub(A, Mul, "urem.sub");
        break;
      }
      default:
        continue;
      }

      // Скопировать DebugLoc на созданные инструкции (если есть)
      if (Instruction *DivI = dyn_cast<Instruction>(Div))
        DivI->setDebugLoc(I->getDebugLoc());
      if (Instruction *MulI = dyn_cast<Instruction>(Mul))
        MulI->setDebugLoc(I->getDebugLoc());
      if (Instruction *SubI = dyn_cast<Instruction>(Sub))
        SubI->setDebugLoc(I->getDebugLoc());

      // Заменяем и удаляем старую инструкцию
      I->replaceAllUsesWith(Sub);
      I->eraseFromParent();
      Changed = true;
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};
} // namespace

// Явно экспортируем точку входа плагина.
// Используем GCC/Clang visibility attribute, чтобы не требовать правок CMake.
extern "C" __attribute__((visibility("default"))) PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "DecomposeRemPass: llvmGetPassPluginInfo called\n";
  return {
      LLVM_PLUGIN_API_VERSION, "DecomposeRemPass", "0.1", [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) -> bool {
              errs() << "DecomposeRemPass: pipeline callback called with name '"
                     << Name << "'\n";
              if (Name == "example") {
                errs() << "DecomposeRemPass: adding pass to pipeline\n";
                FPM.addPass(DecomposeRemPass{});
                return true;
              }
              return false;
            });
      }};
}
