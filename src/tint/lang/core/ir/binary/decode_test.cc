// Copyright 2026 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "src/tint/lang/core/ir/binary/decode.h"

#include <gmock/gmock.h>

#include <utility>

#include "src/tint/lang/core/ir/binary/encode.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/utils/macros/compiler.h"

TINT_BEGIN_DISABLE_PROTOBUF_WARNINGS();
#include "src/tint/utils/protos/ir/ir.pb.h"
TINT_END_DISABLE_PROTOBUF_WARNINGS();

namespace tint::core::ir::binary {
namespace {

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

using IRBinaryDecodeTest = IRTestParamHelper<testing::Test>;

TEST_F(IRBinaryDecodeTest, InvalidFunctionName) {
    auto* fn = b.Function("Function", ty.void_());
    b.Append(fn->Block(), [&] { b.Return(fn); });

    auto res = EncodeToProto(mod);
    ASSERT_EQ(res, Success);

    auto pb_mod = std::move(res.Get());
    pb_mod->mutable_functions(0)->set_name("0invalid");  // Starts with a digit

    auto decoded = Decode(*pb_mod);
    EXPECT_NE(decoded, Success);
    EXPECT_THAT(decoded.Failure().reason,
                testing::HasSubstr("function name '0invalid' is not a valid WGSL identifier"));
}

TEST_F(IRBinaryDecodeTest, InvalidStructName) {
    Vector members{
        ty.Get<core::type::StructMember>(b.ir.symbols.New("a"), ty.i32(), 0u, 0u, 4u, 4u,
                                         core::IOAttributes{}),
    };
    auto* S = ty.Struct(b.ir.symbols.New("S"), std::move(members));
    b.Append(b.ir.root_block, [&] { b.Var(ty.ptr<function, read_write>(S)); });

    auto res = EncodeToProto(mod);
    ASSERT_EQ(res, Success);

    auto pb_mod = std::move(res.Get());
    pb_mod->mutable_types(pb_mod->types_size() - 1)->mutable_struct_()->set_name("__invalid");

    auto decoded = Decode(*pb_mod);
    EXPECT_NE(decoded, Success);
    EXPECT_THAT(decoded.Failure().reason,
                testing::HasSubstr("struct name '__invalid' is not a valid WGSL identifier"));
}

}  // namespace
}  // namespace tint::core::ir::binary
