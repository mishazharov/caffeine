#ifndef CAFFEINE_INTERP_INTERPRETER_H
#define CAFFEINE_INTERP_INTERPRETER_H

#include <memory>

#include "caffeine/IR/Assertion.h"
#include "caffeine/Interpreter/Executor.h"
#include "caffeine/Interpreter/FailureLogger.h"
#include "caffeine/Interpreter/Options.h"
#include "caffeine/Support/Assert.h"

#include <llvm/IR/InstVisitor.h>

namespace caffeine {

enum class ExecutionResult { Continue, Stop };

class Interpreter : public llvm::InstVisitor<Interpreter, ExecutionResult> {
private:
  Context* ctx;
  Executor* queue;
  FailureLogger* logger;
  InterpreterOptions options;

public:
  /**
   * The interpreter constructor needs an executor and context as well as a way
   * to log assertion failures.
   */
  Interpreter(Executor* queue, Context* ctx, FailureLogger* logger,
              const InterpreterOptions& options = InterpreterOptions());

  void execute();

  ExecutionResult visitInstruction(llvm::Instruction& inst);

  ExecutionResult visitAdd(llvm::BinaryOperator& op);
  ExecutionResult visitSub(llvm::BinaryOperator& op);
  ExecutionResult visitMul(llvm::BinaryOperator& op);
  ExecutionResult visitUDiv(llvm::BinaryOperator& op);
  ExecutionResult visitSDiv(llvm::BinaryOperator& op);
  ExecutionResult visitURem(llvm::BinaryOperator& op);
  ExecutionResult visitSRem(llvm::BinaryOperator& op);

  ExecutionResult visitShl(llvm::BinaryOperator& op);
  ExecutionResult visitLShr(llvm::BinaryOperator& op);
  ExecutionResult visitAShr(llvm::BinaryOperator& op);
  ExecutionResult visitAnd(llvm::BinaryOperator& op);
  ExecutionResult visitOr(llvm::BinaryOperator& op);
  ExecutionResult visitXor(llvm::BinaryOperator& op);
  ExecutionResult visitNot(llvm::BinaryOperator& op);

  ExecutionResult visitFAdd(llvm::BinaryOperator& op);
  ExecutionResult visitFSub(llvm::BinaryOperator& op);
  ExecutionResult visitFMul(llvm::BinaryOperator& op);
  ExecutionResult visitFDiv(llvm::BinaryOperator& op);

  ExecutionResult visitICmpInst(llvm::ICmpInst& icmp);
  ExecutionResult visitFCmpInst(llvm::FCmpInst& fcmp);

  ExecutionResult visitTrunc(llvm::TruncInst& trunc);

  ExecutionResult visitSExt(llvm::SExtInst& sext);
  ExecutionResult visitZExt(llvm::ZExtInst& zext);
  ExecutionResult visitIntToPtrInst(llvm::IntToPtrInst& inttoptr);
  ExecutionResult visitPtrToIntInst(llvm::PtrToIntInst& ptrtoint);

  ExecutionResult visitBitCastInst(llvm::BitCastInst& bitcast);

  ExecutionResult visitPHINode(llvm::PHINode& node);
  ExecutionResult visitBranchInst(llvm::BranchInst& inst);
  ExecutionResult visitReturnInst(llvm::ReturnInst& inst);
  ExecutionResult visitCallInst(llvm::CallInst& inst);
  ExecutionResult visitSelectInst(llvm::SelectInst& inst);
  ExecutionResult visitIntrinsicInst(llvm::IntrinsicInst& inst);

  ExecutionResult visitGetElementPtrInst(llvm::GetElementPtrInst& inst);
  ExecutionResult visitLoadInst(llvm::LoadInst& inst);
  ExecutionResult visitStoreInst(llvm::StoreInst& inst);

  ExecutionResult visitInsertElementInst(llvm::InsertElementInst& inst);
  ExecutionResult visitExtractElementInst(llvm::ExtractElementInst& inst);
  ExecutionResult visitShuffleVectorInst(llvm::ShuffleVectorInst& inst);
  ExecutionResult visitAllocaInst(llvm::AllocaInst& inst);

  ExecutionResult visitMemCpyInst(llvm::MemCpyInst& memcpy);
  ExecutionResult visitMemMoveInst(llvm::MemMoveInst& memmove);
  ExecutionResult visitMemSetInst(llvm::MemSetInst& memset);

private:
  ExecutionResult visitExternFunc(llvm::CallInst& inst);

  ExecutionResult visitAssume(llvm::CallInst& inst);
  ExecutionResult visitAssert(llvm::CallInst& inst);
  ExecutionResult visitSymbolicAlloca(llvm::CallInst& inst);

  ExecutionResult visitMalloc(llvm::CallInst& inst);
  ExecutionResult visitCalloc(llvm::CallInst& inst);
  ExecutionResult visitFree(llvm::CallInst& inst);

  ExecutionResult visitBuiltinResolve(llvm::CallInst& inst);
};

} // namespace caffeine

#endif
