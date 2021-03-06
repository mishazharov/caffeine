
#include "caffeine/IR/Operation.h"
#include <gtest/gtest.h>

using namespace caffeine;

TEST(OperationTests, vtable_is_copied) {
  auto fixed_array = FixedArray::Create(Type::int_ty(32),
                                        {Constant::Create(Type::int_ty(1), 0)});

  auto copy =
      fixed_array->with_new_operands({Constant::Create(Type::int_ty(1), 1)});

  ASSERT_EQ(fixed_array->num_operands(), 1);
  ASSERT_EQ(copy->num_operands(), fixed_array->num_operands());
}
