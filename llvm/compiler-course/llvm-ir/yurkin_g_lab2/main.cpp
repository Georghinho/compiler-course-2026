// main.cpp
// Decompose remainder instructions: frem/srem/urem -> div; mul; sub
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct RemDecomposer : PassInfoMixin<RemDecomposer> {
  // Создаёт выражение r = a - (a / b) * b для заданной инструкции rem.
  // Возвращает Value* с новым выражением (не вставляет в IR, вставляет перед
  // Inst).
  Value *buildReplacement(Instruction *Inst) {
    IRBuilder<> B(Inst);
    Value *A = Inst->getOperand(0);
    Value *Bv = Inst->getOperand(1);

    if (!A || !Bv)
      return nullptr;

    // Сохраняем fast-math флаги, если это FP-оператор
    FastMathFlags FMF;
    if (auto *FPO = dyn_cast<FPMathOperator>(Inst))
      FMF = FPO->getFastMathFlags();
    B.setFastMathFlags(FMF);

    switch (Inst->getOpcode()) {
    case Instruction::FRem: {
      // fdiv; fmul; fsub
      Value *Div = B.CreateFDiv(A, Bv, "rem.fdiv");
      Value *Mul = B.CreateFMul(Div, Bv, "rem.fmul");
      return B.CreateFSub(A, Mul, "rem.fsub");
    }
    case Instruction::SRem: {
      Value *Div = B.CreateSDiv(A, Bv, "rem.sdiv");
      Value *Mul = B.CreateMul(Div, Bv, "rem.smul");
      return B.CreateSub(A, Mul, "rem.ssub");
    }
    case Instruction::URem: {
      Value *Div = B.CreateUDiv(A, Bv, "rem.udiv");
      Value *Mul = B.CreateMul(Div, Bv, "rem.umul");
      return B.CreateSub(A, Mul, "rem.usub");
    }
    default:
      return nullptr;
    }
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    SmallVector<Instruction *, 8> ToReplace;
    // Собираем все rem-инструкции заранее
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        unsigned Op = I.getOpcode();
        if (Op == Instruction::FRem || Op == Instruction::SRem ||
            Op == Instruction::URem)
          ToReplace.push_back(&I);
      }

    if (ToReplace.empty())
      return PreservedAnalyses::all();

    errs() << "RemDecomposer: function " << F.getName() << " - found "
           << ToReplace.size() << " rem(s)\n";

    bool Changed = false;
    for (Instruction *I : ToReplace) {
      // Инструкция могла быть удалена ранее — проверяем
      if (!I->getParent() || I->getFunction() != &F)
        continue;

      Value *NewVal = buildReplacement(I);
      if (!NewVal)
        continue;

      // Если NewVal — инструкция, копируем DebugLoc/metadata
      if (Instruction *NI = dyn_cast<Instruction>(NewVal)) {
        NI->setDebugLoc(I->getDebugLoc());
        // Копируем метаданные, если нужно
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        I->getAllMetadata(MDs);
        for (auto &P : MDs)
          NI->setMetadata(P.first, P.second);
      }

      I->replaceAllUsesWith(NewVal);
      I->eraseFromParent();
      Changed = true;
      errs() << "RemDecomposer: replaced rem in " << F.getName() << "\n";
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

// Экспортируем точку входа плагина с видимостью по умолчанию.
// Имя pipeline — "example" (или замените на нужное в тестах).
extern "C" __attribute__((visibility("default"))) PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "RemDecomposer: llvmGetPassPluginInfo invoked\n";
  return {LLVM_PLUGIN_API_VERSION, "RemDecomposerPlugin", "0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) -> bool {
                  errs() << "RemDecomposer: pipeline callback: '" << Name
                         << "'\n";
                  if (Name == "example" || Name == "decompose-rem" ||
                      Name == "example_LLVM_IR") {
                    FPM.addPass(RemDecomposer());
                    errs() << "RemDecomposer: pass added to pipeline\n";
                    return true;
                  }
                  return false;
                });
          }};
}
