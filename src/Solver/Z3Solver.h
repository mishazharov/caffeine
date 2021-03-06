
#pragma once

#include "caffeine/Solver/Z3Solver.h"

#include "caffeine/IR/Assertion.h"
#include "caffeine/IR/Operation.h"
#include "caffeine/IR/Value.h"
#include "caffeine/IR/Visitor.h"

#include <z3++.h>

#include <limits>
#include <string>
#include <unordered_map>
#include <variant>

namespace caffeine {

class Z3Model : public Model {
public:
  using SymbolName = std::variant<std::string, uint64_t>;
  using ConstMap = std::unordered_map<SymbolName, z3::expr>;

private:
  z3::model model;
  ConstMap constants;

public:
  Z3Model(SolverResult result, const z3::model& model, const ConstMap& map);
  Z3Model(SolverResult result, const z3::model& model, ConstMap&& map);

  Value lookup(const Symbol& symbol, std::optional<size_t> size) const override;
};

class Z3OpVisitor : public ConstOpVisitor<Z3OpVisitor, z3::expr> {
public:
private:
  z3::context* ctx;
  z3::solver* solver;
  Z3Model::ConstMap& constMap;
  std::unordered_map<const Operation*, z3::expr> cache;
  // Used for temporary constants that are needed as an implementation detail
  // but aren't otherwise exposed to clients.
  unsigned tmpConstNum = (1u << 30) / 2;

  static constexpr unsigned tmpConstMax = (1u << 30) - 1;

public:
  Z3OpVisitor(z3::solver* ctx, Z3Model::ConstMap& constMap);

  z3::expr visit(const Operation& op);
  z3::expr visit(const Operation* op) {
    return visit(*op);
  }

  z3::expr visitOperation(const Operation& op);

  // clang-format off
  z3::expr visitConstant     (const Constant& op);
  z3::expr visitConstantInt  (const ConstantInt& op);
  z3::expr visitConstantFloat(const ConstantFloat& op);
  z3::expr visitConstantArray(const ConstantArray& op);
  z3::expr visitUndef        (const Undef& op);
  z3::expr visitFixedArray   (const FixedArray& op);

  // Binary operations
  z3::expr visitAdd (const BinaryOp& op);
  z3::expr visitSub (const BinaryOp& op);
  z3::expr visitMul (const BinaryOp& op);
  z3::expr visitUDiv(const BinaryOp& op);
  z3::expr visitSDiv(const BinaryOp& op);
  z3::expr visitURem(const BinaryOp& op);
  z3::expr visitSRem(const BinaryOp& op);
  z3::expr visitAnd (const BinaryOp& op);
  z3::expr visitOr  (const BinaryOp& op);
  z3::expr visitXor (const BinaryOp& op);
  z3::expr visitShl (const BinaryOp& op);
  z3::expr visitLShr(const BinaryOp& op);
  z3::expr visitAShr(const BinaryOp& op);
  z3::expr visitFAdd(const BinaryOp& op);
  z3::expr visitFSub(const BinaryOp& op);
  z3::expr visitFMul(const BinaryOp& op);
  z3::expr visitFDiv(const BinaryOp& op);
  z3::expr visitFRem(const BinaryOp& op);

  z3::expr visitICmp(const ICmpOp& op);
  z3::expr visitFCmp(const FCmpOp& op);

  z3::expr visitSelectOp(const SelectOp& op);

  z3::expr visitTrunc(const UnaryOp& op);

  z3::expr visitZExt(const UnaryOp& op);
  z3::expr visitSExt(const UnaryOp& op);

  z3::expr visitLoadOp(const LoadOp& op);
  z3::expr visitStoreOp(const StoreOp& op);
  z3::expr visitAllocOp(const AllocOp& op);

  // Unary operations
  z3::expr visitNot (const UnaryOp& op);
  z3::expr visitFNeg(const UnaryOp& op);
  z3::expr visitFIsNaN(const UnaryOp& op);
  // clang-format on

  /**
   * When a temporary constant is needed then use this function.
   *
   * It explicitly creates constants with high-integer names that won't collide
   * unless the running program creates more than 2^29 constant names.
   */
  z3::expr next_const(const z3::sort& sort) {
    CAFFEINE_ASSERT(tmpConstNum != tmpConstMax,
                    "ran out of temporary constant names");

    unsigned const_num = tmpConstNum++;
    return ctx->constant(ctx->int_symbol(const_num), sort);
  }
};

// Convert a Z3 expression to an APInt
llvm::APInt z3_to_apint(const z3::expr& expr);

// Convert a Z3 expression to an APFloat
llvm::APFloat z3_to_apfloat(const z3::expr& expr);

} // namespace caffeine
