#include "caffeine/Memory/MemHeap.h"
#include "caffeine/IR/Assertion.h"
#include "caffeine/Interpreter/Context.h"
#include "caffeine/Solver/Z3Solver.h"

#include <vector>

#include <gtest/gtest.h>

using namespace caffeine;

using llvm::LLVMContext;

// LLVM data layout string for x64_64-pc-linux-gnu
static const char* const X86_64_LINUX =
    "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";

std::unique_ptr<llvm::Function> empty_function(llvm::LLVMContext& llvm) {
  auto function = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(llvm), false),
      llvm::GlobalValue::LinkageTypes::PrivateLinkage,
      0 // addrspace
  );

  llvm::BasicBlock::Create(llvm, "entry", function);

  return std::unique_ptr<llvm::Function>(function);
}

class MemHeapTests : public ::testing::Test {
protected:
  llvm::LLVMContext llvm;
  std::shared_ptr<Solver> solver = std::make_shared<Z3Solver>();
  llvm::DataLayout layout{X86_64_LINUX};
  std::unique_ptr<llvm::Function> function = empty_function(llvm);

  OpRef MakeInt(uint64_t value, unsigned AS = 0) {
    return ConstantInt::Create(
        llvm::APInt(layout.getIndexSizeInBits(AS), value));
  }
};

TEST_F(MemHeapTests, resolve_pointer_single) {
  MemHeap heap;
  Context context{function.get(), solver};

  unsigned index_size = layout.getIndexSizeInBits(0);
  auto align = MakeInt(16);
  auto size = Constant::Create(Type::int_ty(index_size), "size");
  auto alloc = heap.allocate(
      size, align,
      AllocOp::Create(size, ConstantInt::Create(llvm::APInt(8, 0xDD))),
      AllocationKind::Alloca, context);
  auto offset = Constant::Create(Type::int_ty(index_size), "offset");

  context.add(ICmpOp::CreateICmp(ICmpOpcode::ULT, offset, size));

  auto ptr = Pointer(BinaryOp::CreateAdd(heap[alloc].address(), offset));

  ASSERT_EQ(context.check(!heap.check_valid(ptr, 0)), SolverResult::UNSAT);

  auto res = heap.resolve(ptr, context);

  ASSERT_EQ(res.size(), 1);
  ASSERT_EQ(res[0].alloc(), alloc);
  ASSERT_EQ(context.check(res[0].check_null(heap)), SolverResult::UNSAT);
}
