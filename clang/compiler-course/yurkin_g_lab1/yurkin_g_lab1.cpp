// yurkin_g_lab1.cpp
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/DenseMap.h"

using namespace clang;

namespace {

enum class ResourceKind { New, Malloc, Fopen };

struct AllocInfo {
  ResourceKind kind;
  SourceLocation loc; // location to report (we use VarDecl->getLocation())
  const VarDecl *var;
  bool freed = false;
  bool reported = false; // true if we already emitted a return-site diagnostic
};

class LeakVisitor final : public RecursiveASTVisitor<LeakVisitor> {
public:
  explicit LeakVisitor(ASTContext *context) : m_context(context) {}

  // VarDecl initializers: T *p = new T; p = (T*)malloc(...); FILE *f =
  // fopen(...);
  bool VisitVarDecl(VarDecl *vd) {
    if (!vd->hasInit())
      return true;

    const Expr *init = vd->getInit();
    init = init->IgnoreParenImpCasts();

    if (const auto *newExpr = dyn_cast<CXXNewExpr>(init)) {
      recordAlloc(vd, ResourceKind::New, vd->getLocation());
    } else if (const auto *call = dyn_cast<CallExpr>(init)) {
      if (const FunctionDecl *fd = call->getDirectCallee()) {
        StringRef name = fd->getName();
        if (name == "malloc") {
          recordAlloc(vd, ResourceKind::Malloc, vd->getLocation());
        } else if (name == "fopen") {
          recordAlloc(vd, ResourceKind::Fopen, vd->getLocation());
        }
      }
    }
    return true;
  }

  // Assignments: p = new T; p = malloc(...); f = fopen(...);
  bool VisitBinaryOperator(BinaryOperator *bo) {
    if (!bo->isAssignmentOp())
      return true;

    const Expr *lhs = bo->getLHS()->IgnoreParenImpCasts();
    const Expr *rhs = bo->getRHS()->IgnoreParenImpCasts();

    const DeclRefExpr *dref = dyn_cast<DeclRefExpr>(lhs);
    if (!dref)
      return true;

    const ValueDecl *vd = dref->getDecl();
    const VarDecl *var = dyn_cast<VarDecl>(vd);
    if (!var)
      return true;

    if (const auto *newExpr = dyn_cast<CXXNewExpr>(rhs)) {
      recordAlloc(var, ResourceKind::New, var->getLocation());
    } else if (const auto *call = dyn_cast<CallExpr>(rhs)) {
      if (const FunctionDecl *fd = call->getDirectCallee()) {
        StringRef name = fd->getName();
        if (name == "malloc") {
          recordAlloc(var, ResourceKind::Malloc, var->getLocation());
        } else if (name == "fopen") {
          recordAlloc(var, ResourceKind::Fopen, var->getLocation());
        }
      }
    }
    return true;
  }

  // delete p;
  bool VisitCXXDeleteExpr(CXXDeleteExpr *del) {
    const Expr *op = del->getArgument();
    if (!op)
      return true;
    op = op->IgnoreParenImpCasts();
    if (const DeclRefExpr *dref = dyn_cast<DeclRefExpr>(op)) {
      if (const VarDecl *var = dyn_cast<VarDecl>(dref->getDecl())) {
        markFreed(var);
      }
    }
    return true;
  }

  // free(...) and fclose(...)
  bool VisitCallExpr(CallExpr *call) {
    if (const FunctionDecl *fd = call->getDirectCallee()) {
      StringRef name = fd->getName();
      if (name == "free" || name == "fclose") {
        if (call->getNumArgs() >= 1) {
          const Expr *arg = call->getArg(0)->IgnoreParenImpCasts();
          if (const DeclRefExpr *dref = dyn_cast<DeclRefExpr>(arg)) {
            if (const VarDecl *var = dyn_cast<VarDecl>(dref->getDecl())) {
              markFreed(var);
            }
          }
        }
      }
    }
    return true;
  }

  // ReturnStmt: если возвращается переменная с незакрытой аллокацией —
  // диагностируем на return
  bool VisitReturnStmt(ReturnStmt *rs) {
    const Expr *ret = rs->getRetValue();
    if (!ret)
      return true;
    ret = ret->IgnoreParenImpCasts();
    if (const DeclRefExpr *dref = dyn_cast<DeclRefExpr>(ret)) {
      if (const VarDecl *var = dyn_cast<VarDecl>(dref->getDecl())) {
        auto it = m_allocs.find(var);
        if (it != m_allocs.end() && !it->second.freed && !it->second.reported) {
          DiagnosticsEngine &DE = m_context->getDiagnostics();
          unsigned DiagID = DE.getCustomDiagID(
              DiagnosticsEngine::Warning,
              "Ресурс для переменной '%0' может быть не освобожден (не "
              "гарантированное освобождение при return)!");
          DE.Report(rs->getBeginLoc(), DiagID) << var->getName();
          // помечаем как "сообщено", чтобы не дублировать в reportLeaks
          it->second.reported = true;
        }
      }
    }
    return true;
  }

  // После обхода TU — сообщаем о всех аллокациях, которые не были помечены как
  // freed или reported
  void reportLeaks() {
    DiagnosticsEngine &DE = m_context->getDiagnostics();
    for (const auto &p : m_allocs) {
      const AllocInfo &info = p.second;
      if (!info.freed && !info.reported) {
        unsigned DiagID = DE.getCustomDiagID(
            DiagnosticsEngine::Warning,
            "Память или ресурс для переменной '%0' не освобождены!");
        DE.Report(info.loc, DiagID) << info.var->getName();
      }
    }
  }

private:
  ASTContext *m_context;
  llvm::DenseMap<const VarDecl *, AllocInfo> m_allocs;

  void recordAlloc(const VarDecl *var, ResourceKind kind, SourceLocation loc) {
    AllocInfo info;
    info.kind = kind;
    // используем location переменной (чтобы совпадало с expected-warning на
    // VarDecl)
    info.loc = loc;
    info.var = var;
    info.freed = false;
    info.reported = false;
    m_allocs[var] = info;
  }

  void markFreed(const VarDecl *var) {
    auto it = m_allocs.find(var);
    if (it != m_allocs.end()) {
      it->second.freed = true;
    }
  }
};

class LeakConsumer final : public ASTConsumer {
public:
  explicit LeakConsumer(ASTContext *context) : m_visitor(context) {}

  void HandleTranslationUnit(ASTContext &context) override {
    m_visitor.TraverseDecl(context.getTranslationUnitDecl());
    m_visitor.reportLeaks();
  }

private:
  LeakVisitor m_visitor;
};

class LeakAction final : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                 llvm::StringRef) override {
    return std::make_unique<LeakConsumer>(&ci.getASTContext());
  }

  bool ParseArgs(const CompilerInstance &ci,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<LeakAction> X("leak_checker",
                                                 "TU-level leak checker");
