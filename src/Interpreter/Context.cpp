#include "caffeine/Interpreter/Context.h"
#include "caffeine/IR/Operation.h"
#include "caffeine/IR/Type.h"
#include "caffeine/Interpreter/StackFrame.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace caffeine {

static void assert_valid_arg(llvm::Type* type) {
  if (type->isIntegerTy() || type->isFloatingPointTy()) {
    return;
  }

  std::string message;
  llvm::raw_string_ostream os{message};
  os << "Unsupported LLVM type: ";
  type->print(os);

  CAFFEINE_ABORT(message);
}

Context::Context(llvm::Function* function, std::shared_ptr<Solver> solver)
    : solver_(std::move(solver)), module_(function->front().getModule()) {
  stack_.emplace_back(function);
  StackFrame& frame = stack_top();

  size_t i = 0;
  for (auto& arg : function->args()) {
    assert_valid_arg(arg.getType());

    std::string name = arg.getName().str();
    boost::trim(name);

    if (name.empty())
      name = fmt::format("arg{}", i);

    frame.insert(&arg, Constant::Create(Type::from_llvm(arg.getType()), name));

    i += 1;
  }
}

Context Context::fork() const {
  return Context{*this};
}

const StackFrame& Context::stack_top() const {
  CAFFEINE_ASSERT(!stack_.empty());
  return stack_.back();
}

StackFrame& Context::stack_top() {
  CAFFEINE_ASSERT(!stack_.empty());
  return stack_.back();
}

const std::vector<StackFrame>& Context::stack() const {
  return stack_;
}

void Context::push(const StackFrame& frame) {
  stack_.push_back(frame);
}
void Context::push(StackFrame&& frame) {
  stack_.push_back(frame);
}
void Context::pop() {
  CAFFEINE_ASSERT(!stack_.empty());

  auto& frame = stack_.back();
  for (auto allocid : frame.allocations) {
    CAFFEINE_ASSERT(heap()[allocid].kind() == AllocationKind::Alloca,
                    "found non-stack allocation on the stack");

    heap().deallocate(allocid);
  }

  stack_.pop_back();
}

bool Context::empty() const {
  return stack_.empty();
}

std::shared_ptr<Solver> Context::solver() const {
  return solver_;
}

llvm::iterator_range<std::vector<Assertion>::const_iterator>
Context::assertions() const {
  return {std::begin(assertions_), std::end(assertions_)};
}

void Context::add(const Assertion& assertion) {
  assertions_.push_back(assertion);
}
void Context::add(Assertion&& assertion) {
  assertions_.push_back(std::move(assertion));
}

std::optional<ContextValue> Context::lookup_const(llvm::Value* value) const {
  if (auto* constant = llvm::dyn_cast_or_null<llvm::Constant>(value))
    return evaluate_constant_const(constant);

  return stack_top().lookup(value);
}

ContextValue Context::lookup(llvm::Value* value) {
  if (auto* constant = llvm::dyn_cast_or_null<llvm::Constant>(value))
    return evaluate_constant(constant);

  return stack_top().lookup(value);
}

SolverResult Context::check(const Assertion& extra) {
  return solver_->check(assertions_, extra);
}
std::unique_ptr<Model> Context::resolve(const Assertion& extra) {
  return solver_->resolve(assertions_, extra);
}

uint64_t Context::next_constant() {
  // Constant numbers greater than 2^29-1 are reserved for the solvers
  // themselves to create internal constants. If this assertion fires
  // then we'll have to revisit the naming allocations.
  CAFFEINE_ASSERT(constant_num_ < (1 << 29),
                  "ran out of temporary constant names");
  return constant_num_++;
}

} // namespace caffeine
