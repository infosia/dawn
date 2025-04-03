// Copyright 2022 The Dawn & Tint Authors
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

#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/helper_test.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/core/type/void.h"
#include "src/tint/utils/symbol/symbol.h"
#include "src/tint/utils/symbol/symbol_table.h"

namespace tint::core::type {
namespace {

using namespace tint::core::number_suffixes;  // NOLINT
using TypeStructTest = TestHelper;

TEST_F(TypeStructTest, Creation) {
    Manager ty;
    auto* s = ty.Get<Struct>(Symbol(), tint::Empty, 4u /* align */, 8u /* size */,
                             16u /* size_no_padding */);
    EXPECT_EQ(s->Align(), 4u);
    EXPECT_EQ(s->Size(), 8u);
    EXPECT_EQ(s->SizeNoPadding(), 16u);
}

TEST_F(TypeStructTest, Equals) {
    Manager ty;
    auto* a = ty.Get<Struct>(Symbol(1, GenerationID(), "a"), tint::Empty, 4u /* align */,
                             4u /* size */, 4u /* size_no_padding */);
    auto* b = ty.Get<Struct>(Symbol(2, GenerationID(), "b"), tint::Empty, 4u /* align */,
                             4u /* size */, 4u /* size_no_padding */);

    EXPECT_TRUE(a->Equals(*a));
    EXPECT_FALSE(a->Equals(*b));
    EXPECT_FALSE(a->Equals(Void{}));
}

TEST_F(TypeStructTest, FriendlyName) {
    Manager ty;
    auto* s = ty.Get<Struct>(Symbol(1, GenerationID(), "my_struct"), tint::Empty, 4u /* align */,
                             4u /* size */, 4u /* size_no_padding */);
    EXPECT_EQ(s->FriendlyName(), "my_struct");
}

TEST_F(TypeStructTest, Clone) {
    auto id = GenerationID::New();
    SymbolTable syms{id};
    Manager ty;
    core::IOAttributes attrs_location_2;
    attrs_location_2.location = 2;

    auto* s = ty.Get<Struct>(
        syms.New("my_struct"),
        tint::Vector{
            ty.Get<StructMember>(syms.New("b"), ty.vec3<f32>(), 0u, 0u, 16u, 12u, attrs_location_2),
            ty.Get<StructMember>(syms.New("a"), ty.i32(), 1u, 16u, 4u, 4u, core::IOAttributes{})},
        4u /* align */, 8u /* size */, 16u /* size_no_padding */);

    auto new_id = GenerationID::New();
    SymbolTable new_st{new_id};

    core::type::Manager mgr;
    core::type::CloneContext ctx{{&syms}, {&new_st, &mgr}};

    auto* st = s->Clone(ctx);

    EXPECT_TRUE(new_st.Get("my_struct").IsValid());
    EXPECT_EQ(st->Name().Name(), "my_struct");

    EXPECT_EQ(st->Align(), 4u);
    EXPECT_EQ(st->Size(), 8u);
    EXPECT_EQ(st->SizeNoPadding(), 16u);

    auto members = st->Members();
    ASSERT_EQ(members.Length(), 2u);

    EXPECT_EQ(members[0]->Name().Name(), "b");
    EXPECT_TRUE(members[0]->Type()->Is<Vector>());
    EXPECT_EQ(members[0]->Index(), 0u);
    EXPECT_EQ(members[0]->Offset(), 0u);
    EXPECT_EQ(members[0]->Align(), 16u);
    EXPECT_EQ(members[0]->Size(), 12u);

    EXPECT_EQ(members[1]->Name().Name(), "a");
    EXPECT_TRUE(members[1]->Type()->Is<I32>());
    EXPECT_EQ(members[1]->Index(), 1u);
    EXPECT_EQ(members[1]->Offset(), 16u);
    EXPECT_EQ(members[1]->Align(), 4u);
    EXPECT_EQ(members[1]->Size(), 4u);
}

}  // namespace
}  // namespace tint::core::type
