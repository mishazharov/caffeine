#include "caffeine/Solver/Solver.h"
#include "caffeine/IR/Assertion.h"
#include "caffeine/IR/Value.h"
#include "caffeine/IR/Visitor.h"
#include "caffeine/Interpreter/Context.h"

#include <fmt/format.h>

namespace caffeine {

class ExprEvaluator : public ConstOpVisitor<ExprEvaluator, Value> {
public:
  ExprEvaluator(const Model* model) : model_(model) {}

  Value visitOperation(const Operation& op) {
    CAFFEINE_ABORT(fmt::format("Unknown operation: {}", op.opcode_name()));
  }

  Value visitConstant(const Constant& op) {
    Value value = model_->lookup(op.symbol());
    CAFFEINE_ASSERT(value.type() != Type::void_ty());
    return value;
  }
  Value visitConstantArray(const ConstantArray& op) {
    size_t size = visit(*op.size()).apint().getLimitedValue(SIZE_MAX);
    Value value = model_->lookup(op.symbol(), size);
    CAFFEINE_ASSERT(value.type() != Type::void_ty());
    return value;
  }
  Value visitConstantInt(const ConstantInt& op) {
    return op.value();
  }
  Value visitConstantFloat(const ConstantFloat& op) {
    return op.value();
  }
  Value visitFixedArray(const FixedArray& op) {
    auto size_val = op.size();
    uint64_t size = visit(*size_val).apint().getLimitedValue();
    uint32_t bitwidth = size_val->type().bitwidth();

    CAFFEINE_ASSERT(size <= (uint64_t)SIZE_MAX);

    std::vector<char> bytes;
    bytes.reserve(size);

    for (uint64_t i = 0; i < size; ++i) {
      auto val = visit(*op.data()[i]).apint();

      CAFFEINE_ASSERT(val.getBitWidth() == 8);

      bytes.push_back(static_cast<char>(
          val.getLimitedValue(std::numeric_limits<uint8_t>::max())));
    }

    return Value(SharedArray(std::move(bytes)), Type::int_ty(bitwidth));
  }

#define DECL_BINOP(opcode, func)                                               \
  Value visit##opcode(const BinaryOp& op) {                                    \
    return Value::func(visit(op[0]), visit(op[1]));                            \
  }                                                                            \
  static_assert(true)

  DECL_BINOP(Add, bvadd);
  DECL_BINOP(Sub, bvsub);
  DECL_BINOP(Mul, bvmul);
  DECL_BINOP(UDiv, bvudiv);
  DECL_BINOP(SDiv, bvsdiv);
  DECL_BINOP(URem, bvurem);
  DECL_BINOP(SRem, bvsrem);
  DECL_BINOP(And, bvand);
  DECL_BINOP(Or, bvor);
  DECL_BINOP(Xor, bvxor);
  DECL_BINOP(Shl, bvshl);
  DECL_BINOP(LShr, bvlshr);
  DECL_BINOP(AShr, bvashr);
  DECL_BINOP(FAdd, fadd);
  DECL_BINOP(FSub, fsub);
  DECL_BINOP(FMul, fmul);
  DECL_BINOP(FDiv, fdiv);
  DECL_BINOP(FRem, frem);

  DECL_BINOP(Load, load);

  Value visitNot(const UnaryOp& op) {
    return Value::bvnot(visit(op[0]));
  }
  Value visitFNeg(const UnaryOp& op) {
    return Value::fneg(visit(op[0]));
  }
  Value visitFIsNaN(const UnaryOp& op) {
    return Value::FIsNaN(visit(op[0]));
  }

  Value visitSelectOp(const SelectOp& select) {
    return visit(select[0]).apint() == 1 ? visit(select[1]) : visit(select[2]);
  }

  Value visitTrunc(const UnaryOp& op) {
    return Value::trunc(visit(op[0]), op.type().bitwidth());
  }
  Value visitZExt(const UnaryOp& op) {
    return Value::zext(visit(op[0]), op.type().bitwidth());
  }
  Value visitSExt(const UnaryOp& op) {
    return Value::sext(visit(op[0]), op.type().bitwidth());
  }

  Value visitBitcast(const UnaryOp& op) {
    return Value::bitcast(visit(op[0]), op.type());
  }

  Value visitLoad(const LoadOp& op) {
    return Value::load(visit(op[0]), visit(op[1]));
  }
  Value visitStore(const StoreOp& op) {
    return Value::store(visit(op[0]), visit(op[1]), visit(op[2]));
  }

private:
  const Model* model_;
};

Model::Model(SolverResult result) : result_(result) {}

Value Model::evaluate(const Operation& expr) const {
  return ExprEvaluator(this).visit(expr);
}

Value Model::evaluate(const ContextValue& expr, Context& ctx) const {
  if (expr.is_scalar()) {
    return ExprEvaluator(this).visit(*expr.scalar());
  } else if (expr.is_pointer()) {
    auto ptr_val = expr.pointer().value(ctx.heap());
    return ExprEvaluator(this).visit(*ptr_val);
  } else if (expr.is_vector()) {
    const auto& vec = expr.vector();
    std::vector<Value> nested_arr;
    nested_arr.resize(vec.size());

    std::transform(vec.begin(), vec.end(), nested_arr.begin(),
                   [&](const auto& i) -> Value { return evaluate(i, ctx); });

    return Value(std::move(nested_arr));
  }

  CAFFEINE_UNREACHABLE();
}

SolverResult Solver::check(std::vector<Assertion>& assertions) {
  return check(assertions, Assertion());
}

SolverResult Solver::check(std::vector<Assertion>& assertions,
                           const Assertion& extra) {
  return resolve(assertions, extra)->result();
}

std::unique_ptr<Model> Solver::resolve(std::vector<Assertion>& assertions) {
  return resolve(assertions, Assertion());
}

EmptyModel::EmptyModel(SolverResult result) : Model(result) {
  CAFFEINE_ASSERT(result != SolverResult::SAT);
}

Value EmptyModel::lookup(const Symbol&, std::optional<size_t>) const {
  CAFFEINE_ABORT("Model was empty");
}

} // namespace caffeine
