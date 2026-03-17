// Copyright 2023 The Dawn & Tint Authors
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

#include "src/tint/lang/wgsl/reader/program_to_ir/ir_program_test.h"

namespace tint::wgsl::reader {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using ProgramToIRLetTest = helpers::IRProgramTest;

TEST_F(ProgramToIRLetTest, Constant) {
    WrapInFunction(Let("a", Expr(42_i)));

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"(%test_function = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %a:i32 = let 42i
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, BinaryOp) {
    WrapInFunction(Let("a", Add(1_i, 2_i)));

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"(%test_function = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %a:i32 = let 3i
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, Chain) {
    WrapInFunction(Let("a", Expr(1_i)),  //
                   Let("b", Expr("a")),  //
                   Let("c", Expr("b")));

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"(%test_function = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %a:i32 = let 1i
    %b:i32 = let %a
    %c:i32 = let %b
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, TextureAssignment) {
    GlobalVar("t", ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32()),
              Vector{Group(2_u), Binding(3_u)});
    WrapInFunction(Let("a", Expr("t")),  //
                   Call("textureLoad", Expr("a"), Call<vec2<i32>>(), 0_u));

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"($B1: {  # root
  %t:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(2, 3)
}

%test_function = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:texture_2d<f32> = load %t
    %4:vec4<f32> = textureLoad %3, vec2<i32>(0i), 0u
    %tint_symbol:vec4<f32> = let %4
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, TextureAssignment_FilterableToUnfilterable) {
    GlobalVar("t",
              ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32(),
                                 core::TextureFilterable::kFilterable),
              Vector{Group(2_u), Binding(3_u)});
    WrapInFunction(Let("a",
                       ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32(),
                                          core::TextureFilterable::kUnfilterable),
                       Expr("t")),  //
                   Call("textureLoad", Expr("a"), Call<vec2<i32>>(), 0_u));

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"($B1: {  # root
  %t:ptr<handle, texture_2d<f32, filterable>, read> = var undef @binding_point(2, 3)
}

%test_function = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:texture_2d<f32, filterable> = load %t
    %4:texture_2d<f32, unfilterable> = texture_convert %3
    %4:vec4<f32> = textureLoad %4, vec2<i32>(0i), 0u
    %tint_symbol:vec4<f32> = let %4
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, SamplerAssignment) {
    GlobalVar("t", ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32()),
              Vector{Group(1_u), Binding(3_u)});
    GlobalVar("s", ty.sampler(core::type::SamplerKind::kSampler), Vector{Group(2_u), Binding(3_u)});

    auto stmts = Vector{WrapInStatement(Let("a", Expr("s"))),  //
                        WrapInStatement(Call("textureSample", Expr("t"), Expr("a"),
                                             Call<vec2<f32>>(), Call<vec2<i32>>()))};

    Func("test_function", {}, ty.void_(), std::move(stmts),
         Vector{create<ast::StageAttribute>(ast::PipelineStage::kFragment)});

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"($B1: {  # root
  %t:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(1, 3)
  %s:ptr<handle, sampler, read> = var undef @binding_point(2, 3)
}

%test_function = @fragment func():void {
  $B2: {
    %4:sampler = load %s
    %5:texture_2d<f32> = load %t
    %6:vec4<f32> = textureSample %5, %4, vec2<f32>(0.0f), vec2<i32>(0i)
    %tint_symbol:vec4<f32> = let %6
    ret
  }
}
)");
}

TEST_F(ProgramToIRLetTest, SamplerAssignmentNonFilteringFromFiltering) {
    GlobalVar("t", ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32()),
              Vector{Group(1_u), Binding(3_u)});
    GlobalVar("s", ty.sampler(core::SamplerFiltering::kFiltering),
              Vector{Group(2_u), Binding(3_u)});

    auto stmts = Vector{
        WrapInStatement(Let("a", ty.sampler(core::SamplerFiltering::kNonFiltering), Expr("s"))),  //
        WrapInStatement(
            Call("textureSample", Expr("t"), Expr("a"), Call<vec2<f32>>(), Call<vec2<i32>>()))};

    Func("test_function", {}, ty.void_(), std::move(stmts),
         Vector{create<ast::StageAttribute>(ast::PipelineStage::kFragment)});

    auto m = Build();
    ASSERT_EQ(m, Success);

    EXPECT_EQ(Dis(m.Get()),
              R"($B1: {  # root
  %t:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(1, 3)
  %s:ptr<handle, sampler<filtering>, read> = var undef @binding_point(2, 3)
}

%test_function = @fragment func():void {
  $B2: {
    %4:sampler<filtering> = load %s
    %5:sampler<non_filtering> = sampler_convert %4
    %6:texture_2d<f32> = load %t
    %7:vec4<f32> = textureSample %6, %5, vec2<f32>(0.0f), vec2<i32>(0i)
    %tint_symbol:vec4<f32> = let %7
    ret
  }
}
)");
}

}  // namespace
}  // namespace tint::wgsl::reader
