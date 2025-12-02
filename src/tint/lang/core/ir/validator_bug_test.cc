#include "src/tint/lang/core/ir/validator_test.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/builder.h"

namespace tint::core::ir {

using namespace tint::core::fluent_types;

using IR_ValidatorBugTest = IR_ValidatorTest;

TEST_F(IR_ValidatorBugTest, DiscardInHelperCalledByVertex) {
    auto* helper2 = b.Function("helper2", ty.void_());
    b.Append(helper2->Block(), [&] {
        b.Discard();
        b.Return(helper2);
    });

    auto* helper1 = b.Function("helper1", ty.void_());
    b.Append(helper1->Block(), [&] {
        b.Call(helper2);
        b.Return(helper1);
    });

    auto* ep = VertexEntryPoint();
    b.Append(ep->Block(), [&] {
        b.Call(helper1);
        b.Return(ep, b.Construct(ty.vec4<f32>()));
    });

    auto res = ir::Validate(mod);
    ASSERT_NE(res, Success);
    EXPECT_THAT(res.Failure().reason, testing::HasSubstr("discard: cannot be called in non-fragment entry point"));
}

} // namespace tint::core::ir
