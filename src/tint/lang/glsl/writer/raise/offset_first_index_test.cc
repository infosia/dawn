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

#include "src/tint/lang/glsl/writer/raise/offset_first_index.h"

#include "gtest/gtest.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/glsl/writer/raise/add_immediate_vars.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::glsl::writer::raise {
namespace {

class GlslWriter_OffsetFirstIndexTest : public core::ir::transform::TransformTest {
  public:
    GlslWriter_OffsetFirstIndexTest() {
        capabilities.Add(core::ir::Capability::kAllowMultipleImmediateVars);
    }
};

TEST_F(GlslWriter_OffsetFirstIndexTest, NoModify_NoBuiltins) {
    auto* func = b.Function("foo", ty.vec4f(), core::ir::Function::PipelineStage::kVertex);
    func->SetReturnBuiltin(core::BuiltinValue::kPosition);
    b.Append(func->Block(), [&] {  //
        b.Return(func, b.Zero(ty.vec4f()));
    });

    auto* src = R"(
%foo = @vertex func():vec4<f32> [@position] {
  $B1: {
    ret vec4<f32>(0.0f)
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    core::ir::transform::AddImmediateVarsConfig immediate_vars_config;
    auto immediate_vars = AddImmediateVars(mod, immediate_vars_config);
    EXPECT_EQ(immediate_vars, Success);

    OffsetFirstIndexConfig config;
    config.first_vertex = immediate_vars->first_vertex;
    config.first_instance = immediate_vars->first_instance;
    Run(OffsetFirstIndex, config);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_OffsetFirstIndexTest, NoModify_BuiltinsWithNoOffsets) {
    auto* vertex_idx = b.Var("vertex_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    vertex_idx->SetBuiltin(core::BuiltinValue::kVertexIndex);
    mod.root_block->Append(vertex_idx);

    auto* instance_idx = b.Var("instance_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    instance_idx->SetBuiltin(core::BuiltinValue::kInstanceIndex);
    mod.root_block->Append(instance_idx);

    auto* func = b.Function("foo", ty.vec4f(), core::ir::Function::PipelineStage::kVertex);
    func->SetReturnBuiltin(core::BuiltinValue::kPosition);

    b.Append(func->Block(), [&] {
        auto* vertex = b.Load(vertex_idx);
        auto* instance = b.Load(instance_idx);
        b.Let("add", b.Add(vertex, instance));
        b.Return(func, b.Zero(ty.vec4f()));
    });

    auto* src = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %4:u32 = load %vertex_index
    %5:u32 = load %instance_index
    %6:u32 = add %4, %5
    %add:u32 = let %6
    ret vec4<f32>(0.0f)
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    core::ir::transform::AddImmediateVarsConfig immediate_vars_config;
    auto immediate_vars = AddImmediateVars(mod, immediate_vars_config);
    EXPECT_EQ(immediate_vars, Success);

    OffsetFirstIndexConfig config{};
    Run(OffsetFirstIndex, config);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_OffsetFirstIndexTest, VertexOffset) {
    auto* vertex_idx = b.Var("vertex_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    vertex_idx->SetBuiltin(core::BuiltinValue::kVertexIndex);
    mod.root_block->Append(vertex_idx);

    auto* instance_idx = b.Var("instance_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    instance_idx->SetBuiltin(core::BuiltinValue::kInstanceIndex);
    mod.root_block->Append(instance_idx);

    auto* func = b.Function("foo", ty.vec4f(), core::ir::Function::PipelineStage::kVertex);
    func->SetReturnBuiltin(core::BuiltinValue::kPosition);

    b.Append(func->Block(), [&] {
        auto* vertex = b.Load(vertex_idx);
        auto* instance = b.Load(instance_idx);
        b.Let("add", b.Add(vertex, instance));
        b.Return(func, b.Zero(ty.vec4f()));
    });

    auto* src = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %4:u32 = load %vertex_index
    %5:u32 = load %instance_index
    %6:u32 = add %4, %5
    %add:u32 = let %6
    ret vec4<f32>(0.0f)
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
  %tint_first_vertex:ptr<immediate, u32, read> = var undef @location(1)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %5:u32 = load %vertex_index
    %6:u32 = load %tint_first_vertex
    %7:u32 = add %5, %6
    %8:u32 = load %instance_index
    %9:u32 = add %7, %8
    %add:u32 = let %9
    ret vec4<f32>(0.0f)
  }
}
)";

    core::ir::transform::AddImmediateVarsConfig immediate_vars_config;
    immediate_vars_config.first_vertex_location = 1;
    auto immediate_vars = AddImmediateVars(mod, immediate_vars_config);
    EXPECT_EQ(immediate_vars, Success);

    OffsetFirstIndexConfig config{};
    config.first_vertex = immediate_vars->first_vertex;
    Run(OffsetFirstIndex, config);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_OffsetFirstIndexTest, InstanceOffset) {
    auto* vertex_idx = b.Var("vertex_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    vertex_idx->SetBuiltin(core::BuiltinValue::kVertexIndex);
    mod.root_block->Append(vertex_idx);

    auto* instance_idx = b.Var("instance_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    instance_idx->SetBuiltin(core::BuiltinValue::kInstanceIndex);
    mod.root_block->Append(instance_idx);

    auto* func = b.Function("foo", ty.vec4f(), core::ir::Function::PipelineStage::kVertex);
    func->SetReturnBuiltin(core::BuiltinValue::kPosition);

    b.Append(func->Block(), [&] {
        auto* vertex = b.Load(vertex_idx);
        auto* instance = b.Load(instance_idx);
        b.Let("add", b.Add(vertex, instance));
        b.Return(func, b.Zero(ty.vec4f()));
    });

    auto* src = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %4:u32 = load %vertex_index
    %5:u32 = load %instance_index
    %6:u32 = add %4, %5
    %add:u32 = let %6
    ret vec4<f32>(0.0f)
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
  %tint_first_instance:ptr<immediate, u32, read> = var undef @location(1)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %5:u32 = load %vertex_index
    %6:u32 = load %instance_index
    %7:u32 = load %tint_first_instance
    %8:u32 = add %6, %7
    %9:u32 = add %5, %8
    %add:u32 = let %9
    ret vec4<f32>(0.0f)
  }
}
)";

    core::ir::transform::AddImmediateVarsConfig immediate_vars_config;
    immediate_vars_config.first_instance_location = 1;
    auto immediate_vars = AddImmediateVars(mod, immediate_vars_config);
    EXPECT_EQ(immediate_vars, Success);

    OffsetFirstIndexConfig config{};
    config.first_instance = immediate_vars->first_instance;
    Run(OffsetFirstIndex, config);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_OffsetFirstIndexTest, VertexAndInstanceOffset) {
    auto* vertex_idx = b.Var("vertex_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    vertex_idx->SetBuiltin(core::BuiltinValue::kVertexIndex);
    mod.root_block->Append(vertex_idx);

    auto* instance_idx = b.Var("instance_index", ty.ptr(core::AddressSpace::kIn, ty.u32()));
    instance_idx->SetBuiltin(core::BuiltinValue::kInstanceIndex);
    mod.root_block->Append(instance_idx);

    auto* func = b.Function("foo", ty.vec4f(), core::ir::Function::PipelineStage::kVertex);
    func->SetReturnBuiltin(core::BuiltinValue::kPosition);

    b.Append(func->Block(), [&] {
        auto* vertex = b.Load(vertex_idx);
        auto* instance = b.Load(instance_idx);
        b.Let("add", b.Add(vertex, instance));
        b.Return(func, b.Zero(ty.vec4f()));
    });

    auto* src = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %4:u32 = load %vertex_index
    %5:u32 = load %instance_index
    %6:u32 = add %4, %5
    %add:u32 = let %6
    ret vec4<f32>(0.0f)
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %vertex_index:ptr<__in, u32, read> = var undef @builtin(vertex_index)
  %instance_index:ptr<__in, u32, read> = var undef @builtin(instance_index)
  %tint_first_vertex:ptr<immediate, u32, read> = var undef @location(1)
  %tint_first_instance:ptr<immediate, u32, read> = var undef @location(2)
}

%foo = @vertex func():vec4<f32> [@position] {
  $B2: {
    %6:u32 = load %vertex_index
    %7:u32 = load %tint_first_vertex
    %8:u32 = add %6, %7
    %9:u32 = load %instance_index
    %10:u32 = load %tint_first_instance
    %11:u32 = add %9, %10
    %12:u32 = add %8, %11
    %add:u32 = let %12
    ret vec4<f32>(0.0f)
  }
}
)";

    core::ir::transform::AddImmediateVarsConfig immediate_vars_config;
    immediate_vars_config.first_vertex_location = 1;
    immediate_vars_config.first_instance_location = 2;
    auto immediate_vars = AddImmediateVars(mod, immediate_vars_config);
    EXPECT_EQ(immediate_vars, Success);

    OffsetFirstIndexConfig config{};
    config.first_vertex = immediate_vars->first_vertex;
    config.first_instance = immediate_vars->first_instance;
    Run(OffsetFirstIndex, config);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::glsl::writer::raise
