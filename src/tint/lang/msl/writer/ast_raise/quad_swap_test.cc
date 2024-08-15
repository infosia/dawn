// Copyright 2024 The Dawn & Tint Authors
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

#include "src/tint/lang/msl/writer/ast_raise/quad_swap.h"

#include "src/tint/lang/wgsl/ast/transform/helper_test.h"

namespace tint::msl::writer {
namespace {

using QuadSwapTest = ast::transform::TransformTest;

TEST_F(QuadSwapTest, EmptyModule) {
    auto* src = "";

    EXPECT_FALSE(ShouldRun<QuadSwap>(src));
}

TEST_F(QuadSwapTest, DirectUseQuadSwapX) {
    auto* src = R"(
enable subgroups;

@compute @workgroup_size(64)
fn foo() {
  let x: f32 = quadSwapX(1.f);
}
)";

    auto* expect =
        R"(
enable subgroups;

@internal(quad_shuffle) @internal(disable_validation__function_has_no_body)
fn tint_msl_quad_shuffle(data : f32, quad_lane_id : u32) -> f32

var<private> tint_msl_thread_index_in_quadgroup : u32;

fn tint_msl_quad_swap(e : f32) -> f32 {
  return tint_msl_quad_shuffle(e, (tint_msl_thread_index_in_quadgroup ^ 1u));
}

@compute @workgroup_size(64)
fn foo(@internal(thread_index_in_quadgroup) tint_thread_index_in_quadgroup : u32) {
  {
    tint_msl_thread_index_in_quadgroup = tint_thread_index_in_quadgroup;
  }
  let x : f32 = tint_msl_quad_swap(1.0f);
}
)";

    auto got = Run<QuadSwap>(src);

    EXPECT_EQ(expect, str(got));
}

TEST_F(QuadSwapTest, IndirectUseQuadSwapX) {
    auto* src = R"(
enable subgroups;

fn bar() -> vec4u {
  let expr = vec4u(1u, 1u, 1u, 1u);
  return quadSwapX(expr);
}

@compute @workgroup_size(64)
fn foo() {
  let x: vec4u = bar();
}
)";

    auto* expect =
        R"(
enable subgroups;

@internal(quad_shuffle) @internal(disable_validation__function_has_no_body)
fn tint_msl_quad_shuffle(data : vec4<u32>, quad_lane_id : u32) -> vec4<u32>

var<private> tint_msl_thread_index_in_quadgroup : u32;

fn tint_msl_quad_swap(e : vec4<u32>) -> vec4<u32> {
  return tint_msl_quad_shuffle(e, (tint_msl_thread_index_in_quadgroup ^ 1u));
}

fn bar() -> vec4u {
  let expr = vec4u(1u, 1u, 1u, 1u);
  return tint_msl_quad_swap(expr);
}

@compute @workgroup_size(64)
fn foo(@internal(thread_index_in_quadgroup) tint_thread_index_in_quadgroup : u32) {
  {
    tint_msl_thread_index_in_quadgroup = tint_thread_index_in_quadgroup;
  }
  let x : vec4u = bar();
}
)";

    auto got = Run<QuadSwap>(src);

    EXPECT_EQ(expect, str(got));
}

TEST_F(QuadSwapTest, DirectUseQuadSwapY) {
    auto* src = R"(
enable subgroups;

@compute @workgroup_size(64)
fn foo() {
  let x: i32 = quadSwapY(1i);
}
)";

    auto* expect =
        R"(
enable subgroups;

@internal(quad_shuffle) @internal(disable_validation__function_has_no_body)
fn tint_msl_quad_shuffle(data : i32, quad_lane_id : u32) -> i32

var<private> tint_msl_thread_index_in_quadgroup : u32;

fn tint_msl_quad_swap(e : i32) -> i32 {
  return tint_msl_quad_shuffle(e, (tint_msl_thread_index_in_quadgroup ^ 16u));
}

@compute @workgroup_size(64)
fn foo(@internal(thread_index_in_quadgroup) tint_thread_index_in_quadgroup : u32) {
  {
    tint_msl_thread_index_in_quadgroup = tint_thread_index_in_quadgroup;
  }
  let x : i32 = tint_msl_quad_swap(1i);
}
)";

    auto got = Run<QuadSwap>(src);

    EXPECT_EQ(expect, str(got));
}

TEST_F(QuadSwapTest, DirectUseQuadSwapDiagonal) {
    auto* src = R"(
enable subgroups;

@compute @workgroup_size(64)
fn foo() {
  let x: i32 = quadSwapDiagonal(1i);
}
)";

    auto* expect =
        R"(
enable subgroups;

@internal(quad_shuffle) @internal(disable_validation__function_has_no_body)
fn tint_msl_quad_shuffle(data : i32, quad_lane_id : u32) -> i32

var<private> tint_msl_thread_index_in_quadgroup : u32;

fn tint_msl_quad_swap(e : i32) -> i32 {
  return tint_msl_quad_shuffle(e, (tint_msl_thread_index_in_quadgroup ^ 17u));
}

@compute @workgroup_size(64)
fn foo(@internal(thread_index_in_quadgroup) tint_thread_index_in_quadgroup : u32) {
  {
    tint_msl_thread_index_in_quadgroup = tint_thread_index_in_quadgroup;
  }
  let x : i32 = tint_msl_quad_swap(1i);
}
)";

    auto got = Run<QuadSwap>(src);

    EXPECT_EQ(expect, str(got));
}

}  // namespace
}  // namespace tint::msl::writer
