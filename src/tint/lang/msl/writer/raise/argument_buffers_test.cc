// Copyright 2025 The Dawn & Tint Authors
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

#include "src/tint/lang/msl/writer/raise/argument_buffers.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/type/binding_array.h"
#include "src/tint/lang/core/type/sampled_texture.h"

namespace tint::msl::writer::raise {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class MslWriter_ArgumentBuffersTest : public core::ir::transform::TransformTest {
  public:
    void SetUp() override {
        capabilities.Add(core::ir::Capability::kAllowPointersAndHandlesInStructures,
                         core::ir::Capability::kAllowPrivateVarsInFunctions,
                         core::ir::Capability::kAllowAnyLetType);
    }
};

TEST_F(MslWriter_ArgumentBuffersTest, NoArgumentBuffers) {
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* var = b.Var<function, i32>("v");
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    %v:ptr<function, i32, read_write> = var undef
    %3:i32 = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, Private_NoChange) {
    auto* var_a = b.Var("a", ty.ptr<private_, i32>());
    auto* var_b = b.Var("b", ty.ptr<private_, i32>());
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_a, b.Add<i32>(load_a, load_b));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<private, i32, read_write> = var undef
  %b:ptr<private, i32, read_write> = var undef
}

%foo = @fragment func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %a, %6
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %a:ptr<private, i32, read_write> = var undef
  %b:ptr<private, i32, read_write> = var undef
}

%foo = @fragment func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %a, %6
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, Workgroup_NoChange) {
    auto* var_a = b.Var("a", ty.ptr<workgroup, i32>());
    auto* var_b = b.Var("b", ty.ptr<workgroup, i32>());
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* func = b.ComputeFunction("foo");
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_a, b.Add<i32>(load_a, load_b));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<workgroup, i32, read_write> = var undef
  %b:ptr<workgroup, i32, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %a, %6
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %a:ptr<workgroup, i32, read_write> = var undef
  %b:ptr<workgroup, i32, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %a, %6
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, Storage) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    auto* var_c = b.Var("c", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    var_c->SetBindingPoint(3, 2);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);
    mod.root_block->Append(var_c);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        auto* load_c = b.Load(var_c);
        b.Store(var_b, b.Add<i32>(b.Add<i32>(load_a, load_b), load_c));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
  %c:ptr<storage, i32, read_write> = var undef @binding_point(3, 2)
}

%foo = @fragment func():void {
  $B2: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = load %c
    %8:i32 = add %5, %6
    %9:i32 = add %8, %7
    store %b, %9
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
  c:ptr<storage, i32, read_write> @offset(0), @id(2)
}

%foo = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %4:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %5:i32 = load %4
    %6:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %7:i32 = load %6
    %8:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 1u
    %9:i32 = load %8
    %10:i32 = add %5, %7
    %11:i32 = add %10, %9
    %12:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %12, %11
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, Uniform) {
    auto* var_a = b.Var("a", ty.ptr<uniform, i32>());
    auto* var_b = b.Var("b", ty.ptr<uniform, i32>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Add<i32>(load_a, load_b);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<uniform, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<uniform, i32, read> = var undef @binding_point(3, 4)
}

%foo = @fragment func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<uniform, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<uniform, i32, read> @offset(0), @id(4)
}

%foo = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %4:ptr<uniform, i32, read> = access %tint_arg_buffer_1, 0u
    %5:i32 = load %4
    %6:ptr<uniform, i32, read> = access %tint_arg_buffer_3, 0u
    %7:i32 = load %6
    %8:i32 = add %5, %7
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, HandleTypes) {
    auto* t = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* var_t = b.Var("t", ty.ptr<handle>(t));
    auto* var_s = b.Var("s", ty.ptr<handle>(ty.sampler()));
    var_t->SetBindingPoint(1, 2);
    var_s->SetBindingPoint(3, 4);
    mod.root_block->Append(var_t);
    mod.root_block->Append(var_s);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_t = b.Load(var_t);
        auto* load_s = b.Load(var_s);
        b.Call<vec4<f32>>(core::BuiltinFn::kTextureSample, load_t, load_s, b.Splat<vec2<f32>>(0_f));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %t:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(1, 2)
  %s:ptr<handle, sampler, read> = var undef @binding_point(3, 4)
}

%foo = @fragment func():void {
  $B2: {
    %4:texture_2d<f32> = load %t
    %5:sampler = load %s
    %6:vec4<f32> = textureSample %4, %5, vec2<f32>(0.0f)
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  t:texture_2d<f32> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  s:sampler @offset(0), @id(4)
}

%foo = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %4:texture_2d<f32> = access %tint_arg_buffer_1, 0u
    %5:sampler = access %tint_arg_buffer_3, 0u
    %6:vec4<f32> = textureSample %4, %5, vec2<f32>(0.0f)
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, HandleTypes_BindingArray) {
    auto* texture_type = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* var_t = b.Var("t", ty.ptr<handle>(ty.binding_array(texture_type, 3u)));
    auto* var_s = b.Var("s", ty.ptr<handle>(ty.binding_array(ty.sampler(), 3u)));
    var_t->SetBindingPoint(1, 2);
    var_s->SetBindingPoint(3, 4);
    mod.root_block->Append(var_t);
    mod.root_block->Append(var_s);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_t = b.Load(b.Access(ty.ptr<handle>(texture_type), var_t, 0_i));
        auto* load_s = b.Load(b.Access(ty.ptr<handle>(ty.sampler()), var_s, 0_i));
        b.Call<vec4<f32>>(core::BuiltinFn::kTextureSample, load_t, load_s, b.Splat<vec2<f32>>(0_f));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %t:ptr<handle, binding_array<texture_2d<f32>, 3>, read> = var undef @binding_point(1, 2)
  %s:ptr<handle, binding_array<sampler, 3>, read> = var undef @binding_point(3, 4)
}

%foo = @fragment func():void {
  $B2: {
    %4:ptr<handle, texture_2d<f32>, read> = access %t, 0i
    %5:texture_2d<f32> = load %4
    %6:ptr<handle, sampler, read> = access %s, 0i
    %7:sampler = load %6
    %8:vec4<f32> = textureSample %5, %7, vec2<f32>(0.0f)
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  t:binding_array<texture_2d<f32>, 3> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  s:binding_array<sampler, 3> @offset(0), @id(4)
}

%foo = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %4:binding_array<texture_2d<f32>, 3> = access %tint_arg_buffer_1, 0u
    %5:texture_2d<f32> = access %4, 0i
    %6:binding_array<sampler, 3> = access %tint_arg_buffer_3, 0u
    %7:sampler = access %6, 0i
    %8:vec4<f32> = textureSample %5, %7, vec2<f32>(0.0f)
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, MultipleAddressSpaces) {
    auto* var_a = b.Var("a", ty.ptr<uniform, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    auto* var_c = b.Var("c", ty.ptr<private_, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);
    mod.root_block->Append(var_c);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        auto* load_c = b.Load(var_c);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, load_c)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<uniform, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
  %c:ptr<private, i32, read_write> = var undef
}

%foo = @fragment func():void {
  $B2: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = load %c
    %8:i32 = add %6, %7
    %9:i32 = add %5, %8
    store %b, %9
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<uniform, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

$B1: {  # root
  %c:ptr<private, i32, read_write> = var undef
}

%foo = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %5:ptr<uniform, i32, read> = access %tint_arg_buffer_1, 0u
    %6:i32 = load %5
    %7:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %8:i32 = load %7
    %9:i32 = load %c
    %10:i32 = add %8, %9
    %11:i32 = add %6, %10
    %12:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %12, %11
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, EntryPointHasExistingParameters) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    auto* param = b.FunctionParam<i32>("param");
    param->SetLocation(1_u);
    param->SetInterpolation(core::Interpolation{core::InterpolationType::kFlat});
    func->SetParams({param});
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, param)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%foo = @fragment func(%param:i32 [@location(1), @interpolate(flat)]):void {
  $B2: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = add %6, %param
    %8:i32 = add %5, %7
    store %b, %8
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%foo = @fragment func(%param:i32 [@location(1), @interpolate(flat)], %tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %5:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %6:i32 = load %5
    %7:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %8:i32 = load %7
    %9:i32 = add %8, %param
    %10:i32 = add %6, %9
    %11:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %11, %10
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatUsesVars_NoArgs) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, load_b));
        b.Return(foo);
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(foo);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%foo = func():void {
  $B2: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %b, %6
    ret
  }
}
%main = @fragment func():void {
  $B3: {
    %8:void = call %foo
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%foo = func(%a:ptr<storage, i32, read>, %b:ptr<storage, i32, read_write>):void {
  $B1: {
    %4:i32 = load %a
    %5:i32 = load %b
    %6:i32 = add %4, %5
    store %b, %6
    ret
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %10:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %11:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %12:void = call %foo, %10, %11
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatUsesVars_WithExistingParameters) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* foo = b.Function("foo", ty.void_());
    auto* param = b.FunctionParam<i32>("param");
    foo->SetParams({param});
    b.Append(foo->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, param)));
        b.Return(foo);
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(foo, 42_i);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%foo = func(%param:i32):void {
  $B2: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = add %6, %param
    %8:i32 = add %5, %7
    store %b, %8
    ret
  }
}
%main = @fragment func():void {
  $B3: {
    %10:void = call %foo, 42i
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%foo = func(%param:i32, %a:ptr<storage, i32, read>, %b:ptr<storage, i32, read_write>):void {
  $B1: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = add %6, %param
    %8:i32 = add %5, %7
    store %b, %8
    ret
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %12:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %13:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %14:void = call %foo, 42i, %12, %13
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatUsesVars_HandleTypes) {
    auto* t = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* var_t = b.Var("t", ty.ptr<handle>(t));
    auto* var_s = b.Var("s", ty.ptr<handle>(ty.sampler()));
    var_t->SetBindingPoint(1, 2);
    var_s->SetBindingPoint(3, 4);
    mod.root_block->Append(var_t);
    mod.root_block->Append(var_s);

    auto* foo = b.Function("foo", ty.vec4<f32>());
    auto* param = b.FunctionParam<i32>("param");
    foo->SetParams({param});
    b.Append(foo->Block(), [&] {
        auto* load_t = b.Load(var_t);
        auto* load_s = b.Load(var_s);
        auto* result = b.Call<vec4<f32>>(core::BuiltinFn::kTextureSample, load_t, load_s,
                                         b.Splat<vec2<f32>>(0_f));
        b.Return(foo, result);
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(foo, 42_i);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %t:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(1, 2)
  %s:ptr<handle, sampler, read> = var undef @binding_point(3, 4)
}

%foo = func(%param:i32):vec4<f32> {
  $B2: {
    %5:texture_2d<f32> = load %t
    %6:sampler = load %s
    %7:vec4<f32> = textureSample %5, %6, vec2<f32>(0.0f)
    ret %7
  }
}
%main = @fragment func():void {
  $B3: {
    %9:vec4<f32> = call %foo, 42i
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  t:texture_2d<f32> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  s:sampler @offset(0), @id(4)
}

%foo = func(%param:i32, %t:texture_2d<f32>, %s:sampler):vec4<f32> {
  $B1: {
    %5:vec4<f32> = textureSample %t, %s, vec2<f32>(0.0f)
    ret %5
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %9:texture_2d<f32> = access %tint_arg_buffer_1, 0u
    %10:sampler = access %tint_arg_buffer_3, 0u
    %11:vec4<f32> = call %foo, 42i, %9, %10
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatUsesVars_HandleTypes_BindingArray) {
    auto* texture_type = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* var_t = b.Var("t", ty.ptr<handle>(ty.binding_array(texture_type, 3u)));
    auto* var_s = b.Var("s", ty.ptr<handle>(ty.binding_array(ty.sampler(), 3u)));
    var_t->SetBindingPoint(1, 2);
    var_s->SetBindingPoint(3, 4);
    mod.root_block->Append(var_t);
    mod.root_block->Append(var_s);

    auto* foo = b.Function("foo", ty.vec4<f32>());
    auto* param = b.FunctionParam<i32>("param");
    foo->SetParams({param});
    b.Append(foo->Block(), [&] {
        auto* load_t = b.Load(b.Access(ty.ptr<handle>(texture_type), var_t, 0_i));
        auto* load_s = b.Load(b.Access(ty.ptr<handle>(ty.sampler()), var_s, 0_i));
        auto* result = b.Call<vec4<f32>>(core::BuiltinFn::kTextureSample, load_t, load_s,
                                         b.Splat<vec2<f32>>(0_f));
        b.Return(foo, result);
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Call(foo, 42_i);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %t:ptr<handle, binding_array<texture_2d<f32>, 3>, read> = var undef @binding_point(1, 2)
  %s:ptr<handle, binding_array<sampler, 3>, read> = var undef @binding_point(3, 4)
}

%foo = func(%param:i32):vec4<f32> {
  $B2: {
    %5:ptr<handle, texture_2d<f32>, read> = access %t, 0i
    %6:texture_2d<f32> = load %5
    %7:ptr<handle, sampler, read> = access %s, 0i
    %8:sampler = load %7
    %9:vec4<f32> = textureSample %6, %8, vec2<f32>(0.0f)
    ret %9
  }
}
%main = @fragment func():void {
  $B3: {
    %11:vec4<f32> = call %foo, 42i
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  t:binding_array<texture_2d<f32>, 3> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  s:binding_array<sampler, 3> @offset(0), @id(4)
}

%foo = func(%param:i32, %t:binding_array<texture_2d<f32>, 3>, %s:binding_array<sampler, 3>):vec4<f32> {
  $B1: {
    %5:texture_2d<f32> = access %t, 0i
    %6:sampler = access %s, 0i
    %7:vec4<f32> = textureSample %5, %6, vec2<f32>(0.0f)
    ret %7
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %11:binding_array<texture_2d<f32>, 3> = access %tint_arg_buffer_1, 0u
    %12:binding_array<sampler, 3> = access %tint_arg_buffer_3, 0u
    %13:vec4<f32> = call %foo, 42i, %11, %12
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatUsesVars_OutOfOrder) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, load_b));
        b.Return(foo);
    });

    b.Append(func->Block(), [&] {
        b.Call(foo);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%main = @fragment func():void {
  $B2: {
    %4:void = call %foo
    ret
  }
}
%foo = func():void {
  $B3: {
    %6:i32 = load %a
    %7:i32 = load %b
    %8:i32 = add %6, %7
    store %b, %8
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B1: {
    %4:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %5:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %6:void = call %foo, %4, %5
    ret
  }
}
%foo = func(%a:ptr<storage, i32, read>, %b:ptr<storage, i32, read_write>):void {
  $B2: {
    %10:i32 = load %a
    %11:i32 = load %b
    %12:i32 = add %10, %11
    store %b, %12
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

// Test that we do not add the variables to functions that do not need it.
TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionThatDoesNotUseVars) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* foo = b.Function("foo", ty.i32());
    b.Append(foo->Block(), [&] {  //
        b.Return(foo, 42_i);
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, b.Call(foo))));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%foo = func():i32 {
  $B2: {
    ret 42i
  }
}
%main = @fragment func():void {
  $B3: {
    %5:i32 = load %a
    %6:i32 = load %b
    %7:i32 = call %foo
    %8:i32 = add %6, %7
    %9:i32 = add %5, %8
    store %b, %9
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%foo = func():i32 {
  $B1: {
    ret 42i
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B2: {
    %5:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %6:i32 = load %5
    %7:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %8:i32 = load %7
    %9:i32 = call %foo
    %10:i32 = add %8, %9
    %11:i32 = add %6, %10
    %12:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %12, %11
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

// Test that we *do* add the variables to functions that only have transitive uses.
TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionWithOnlyTransitiveUses) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* bar = b.Function("bar", ty.i32());
    b.Append(bar->Block(), [&] {  //
        b.Return(bar, b.Load(var_a));
    });

    auto* foo = b.Function("foo", ty.i32());
    b.Append(foo->Block(), [&] {  //
        b.Return(foo, b.Call(bar));
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, b.Call(foo))));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%bar = func():i32 {
  $B2: {
    %4:i32 = load %a
    ret %4
  }
}
%foo = func():i32 {
  $B3: {
    %6:i32 = call %bar
    ret %6
  }
}
%main = @fragment func():void {
  $B4: {
    %8:i32 = load %a
    %9:i32 = load %b
    %10:i32 = call %foo
    %11:i32 = add %9, %10
    %12:i32 = add %8, %11
    store %b, %12
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%bar = func(%a:ptr<storage, i32, read>):i32 {
  $B1: {
    %3:i32 = load %a
    ret %3
  }
}
%foo = func(%a_1:ptr<storage, i32, read>):i32 {  # %a_1: 'a'
  $B2: {
    %6:i32 = call %bar, %a_1
    ret %6
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B3: {
    %10:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %11:i32 = load %10
    %12:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %13:i32 = load %12
    %14:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %15:i32 = call %foo, %14
    %16:i32 = add %13, %15
    %17:i32 = add %11, %16
    %18:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %18, %17
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

// Test that we *do* add the structure to functions that only have transitive uses, where that
// function is declared first.
TEST_F(MslWriter_ArgumentBuffersTest, CallFunctionWithOnlyTransitiveUses_OutOfOrder) {
    auto* var_a = b.Var("a", ty.ptr<storage, i32, core::Access::kRead>());
    auto* var_b = b.Var("b", ty.ptr<storage, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);

    auto* foo = b.Function("foo", ty.i32());

    auto* bar = b.Function("bar", ty.i32());
    b.Append(bar->Block(), [&] {  //
        b.Return(bar, b.Load(var_a));
    });

    b.Append(foo->Block(), [&] {  //
        b.Return(foo, b.Call(bar));
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, b.Call(foo))));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<storage, i32, read> = var undef @binding_point(1, 2)
  %b:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
}

%foo = func():i32 {
  $B2: {
    %4:i32 = call %bar
    ret %4
  }
}
%bar = func():i32 {
  $B3: {
    %6:i32 = load %a
    ret %6
  }
}
%main = @fragment func():void {
  $B4: {
    %8:i32 = load %a
    %9:i32 = load %b
    %10:i32 = call %foo
    %11:i32 = add %9, %10
    %12:i32 = add %8, %11
    store %b, %12
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  a:ptr<storage, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  b:ptr<storage, i32, read_write> @offset(0), @id(4)
}

%foo = func(%a:ptr<storage, i32, read>):i32 {
  $B1: {
    %3:i32 = call %bar, %a
    ret %3
  }
}
%bar = func(%a_1:ptr<storage, i32, read>):i32 {  # %a_1: 'a'
  $B2: {
    %6:i32 = load %a_1
    ret %6
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B3: {
    %10:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %11:i32 = load %10
    %12:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %13:i32 = load %12
    %14:ptr<storage, i32, read> = access %tint_arg_buffer_1, 0u
    %15:i32 = call %foo, %14
    %16:i32 = add %13, %15
    %17:i32 = add %11, %16
    %18:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %18, %17
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ArgumentBuffersTest, VarsWithNoNames) {
    auto* var_a = b.Var(ty.ptr<uniform, i32, core::Access::kRead>());
    auto* var_b = b.Var(ty.ptr<storage, i32, core::Access::kReadWrite>());
    auto* var_c = b.Var(ty.ptr<private_, i32, core::Access::kReadWrite>());
    var_a->SetBindingPoint(1, 2);
    var_b->SetBindingPoint(3, 4);
    mod.root_block->Append(var_a);
    mod.root_block->Append(var_b);
    mod.root_block->Append(var_c);

    auto* foo = b.Function("foo", ty.i32());

    auto* bar = b.Function("bar", ty.i32());
    b.Append(bar->Block(), [&] {  //
        b.Return(bar, b.Load(var_a));
    });

    b.Append(foo->Block(), [&] {  //
        b.Return(foo, b.Call(bar));
    });

    auto* func = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* load_a = b.Load(var_a);
        auto* load_b = b.Load(var_b);
        b.Store(var_b, b.Add<i32>(load_a, b.Add<i32>(load_b, b.Call(foo))));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<uniform, i32, read> = var undef @binding_point(1, 2)
  %2:ptr<storage, i32, read_write> = var undef @binding_point(3, 4)
  %3:ptr<private, i32, read_write> = var undef
}

%foo = func():i32 {
  $B2: {
    %5:i32 = call %bar
    ret %5
  }
}
%bar = func():i32 {
  $B3: {
    %7:i32 = load %1
    ret %7
  }
}
%main = @fragment func():void {
  $B4: {
    %9:i32 = load %1
    %10:i32 = load %2
    %11:i32 = call %foo
    %12:i32 = add %10, %11
    %13:i32 = add %9, %12
    store %2, %13
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
tint_arg_buffer_struct_1 = struct @align(1) {
  tint_symbol:ptr<uniform, i32, read> @offset(0), @id(2)
}

tint_arg_buffer_struct_3 = struct @align(1) {
  tint_symbol_1:ptr<storage, i32, read_write> @offset(0), @id(4)
}

$B1: {  # root
  %1:ptr<private, i32, read_write> = var undef
}

%foo = func(%3:ptr<uniform, i32, read>):i32 {
  $B2: {
    %4:i32 = call %bar, %3
    ret %4
  }
}
%bar = func(%6:ptr<uniform, i32, read>):i32 {
  $B3: {
    %7:i32 = load %6
    ret %7
  }
}
%main = @fragment func(%tint_arg_buffer_1:tint_arg_buffer_struct_1 [@buffer(1)], %tint_arg_buffer_3:tint_arg_buffer_struct_3 [@buffer(3)]):void {
  $B4: {
    %11:ptr<uniform, i32, read> = access %tint_arg_buffer_1, 0u
    %12:i32 = load %11
    %13:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    %14:i32 = load %13
    %15:ptr<uniform, i32, read> = access %tint_arg_buffer_1, 0u
    %16:i32 = call %foo, %15
    %17:i32 = add %14, %16
    %18:i32 = add %12, %17
    %19:ptr<storage, i32, read_write> = access %tint_arg_buffer_3, 0u
    store %19, %18
    ret
  }
}
)";

    Run(ArgumentBuffers);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::msl::writer::raise
