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

// tint-tbdr: begin input_attachment-msl

#include "src/tint/lang/msl/writer/raise/input_attachment.h"

#include <unordered_map>
#include <utility>

#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/type/input_attachment.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::msl::writer::raise {
namespace {

using MslWriter_InputAttachmentTest = core::ir::transform::TransformTest;

// Creates a module-scope input_attachment variable.
core::ir::Var* MakeInputAttachment(core::ir::Builder& b,
                                   const core::type::Type* sample_ty,
                                   uint32_t group,
                                   uint32_t binding,
                                   uint32_t index) {
    core::ir::Var* var = nullptr;
    b.Append(b.ir.root_block, [&] {
        var = b.Var("ia", b.ir.Types().ptr(core::AddressSpace::kHandle,
                                            b.ir.Types().Get<core::type::InputAttachment>(sample_ty),
                                            core::Access::kRead));
        var->SetBindingPoint(group, binding);
        var->SetInputAttachmentIndex(index);
    });
    return var;
}

TEST_F(MslWriter_InputAttachmentTest, SingleAttachment) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia = MakeInputAttachment(b, ty.f32(), 1, 0, 3);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        auto* load = b.Load(ia);
        auto* color = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, load);
        b.Store(dce, color);
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<f32>, read_write> = var undef @binding_point(0, 1)
  %tint_input_attachment_0:ptr<private, vec4<f32>, read_write> = var vec4<f32>(0.0f)
}

%f = @fragment func(%tint_input_attachment_0_1:vec4<f32> [@color(0)]):void {  # %tint_input_attachment_0_1: 'tint_input_attachment_0'
  $B2: {
    store %tint_input_attachment_0, %tint_input_attachment_0_1
    %5:vec4<f32> = load %tint_input_attachment_0
    store %dce, %5
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, UnusedAttachmentDropped) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    MakeInputAttachment(b, ty.f32(), 1, 0, 3);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        b.Store(dce, b.Zero(ty.vec4f()));
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<f32>, read_write> = var undef @binding_point(0, 1)
}

%f = @fragment func():void {
  $B2: {
    store %dce, vec4<f32>(0.0f)
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, LoadInHelper) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia = MakeInputAttachment(b, ty.f32(), 1, 0, 3);

    auto* helper = b.Function("load_it", ty.vec4f());
    b.Append(helper->Block(), [&] {
        auto* load = b.Load(ia);
        auto* color = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, load);
        b.Return(helper, color);
    });

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        b.Store(dce, b.Call(ty.vec4f(), helper));
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<f32>, read_write> = var undef @binding_point(0, 1)
  %tint_input_attachment_0:ptr<private, vec4<f32>, read_write> = var vec4<f32>(0.0f)
}

%load_it = func():vec4<f32> {
  $B2: {
    %4:vec4<f32> = load %tint_input_attachment_0
    ret %4
  }
}
%f = @fragment func(%tint_input_attachment_0_1:vec4<f32> [@color(0)]):void {  # %tint_input_attachment_0_1: 'tint_input_attachment_0'
  $B3: {
    store %tint_input_attachment_0, %tint_input_attachment_0_1
    %7:vec4<f32> = call %load_it
    store %dce, %7
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, MultipleAttachments) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia0 = MakeInputAttachment(b, ty.f32(), 1, 0, 0);
    auto* ia1 = MakeInputAttachment(b, ty.f32(), 1, 1, 1);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        auto* c0 = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia0));
        auto* c1 = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia1));
        b.Store(dce, b.Add(c0, c1));
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<f32>, read_write> = var undef @binding_point(0, 1)
  %tint_input_attachment_0:ptr<private, vec4<f32>, read_write> = var vec4<f32>(0.0f)
  %tint_input_attachment_1:ptr<private, vec4<f32>, read_write> = var vec4<f32>(0.0f)
}

%f = @fragment func(%tint_input_attachment_0_1:vec4<f32> [@color(0)], %tint_input_attachment_1_1:vec4<f32> [@color(1)]):void {  # %tint_input_attachment_0_1: 'tint_input_attachment_0', %tint_input_attachment_1_1: 'tint_input_attachment_1'
  $B2: {
    store %tint_input_attachment_1, %tint_input_attachment_1_1
    store %tint_input_attachment_0, %tint_input_attachment_0_1
    %7:vec4<f32> = load %tint_input_attachment_0
    %8:vec4<f32> = load %tint_input_attachment_1
    %9:vec4<f32> = add %7, %8
    store %dce, %9
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}, {BindingPoint{1, 1}, 1}};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, DuplicateColorSlotFails) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia0 = MakeInputAttachment(b, ty.f32(), 1, 0, 0);
    auto* ia1 = MakeInputAttachment(b, ty.f32(), 1, 1, 1);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        auto* c0 = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia0));
        auto* c1 = b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia1));
        b.Store(dce, b.Add(c0, c1));
        b.Return(ep);
    });

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}, {BindingPoint{1, 1}, 0}};
    auto result = InputAttachment(mod, InputAttachmentConfig{map});
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason,
              "input_attachment color index 0 is used by more than one binding point");
}

TEST_F(MslWriter_InputAttachmentTest, U32Attachment) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4u()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia = MakeInputAttachment(b, ty.u32(), 1, 0, 3);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        b.Store(dce, b.Call(ty.vec4u(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia)));
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<u32>, read_write> = var undef @binding_point(0, 1)
  %tint_input_attachment_0:ptr<private, vec4<u32>, read_write> = var vec4<u32>(0u)
}

%f = @fragment func(%tint_input_attachment_0_1:vec4<u32> [@color(0)]):void {  # %tint_input_attachment_0_1: 'tint_input_attachment_0'
  $B2: {
    store %tint_input_attachment_0, %tint_input_attachment_0_1
    %5:vec4<u32> = load %tint_input_attachment_0
    store %dce, %5
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, I32Attachment) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4i()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia = MakeInputAttachment(b, ty.i32(), 1, 0, 3);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        b.Store(dce, b.Call(ty.vec4i(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia)));
        b.Return(ep);
    });

    auto* expect = R"(
$B1: {  # root
  %dce:ptr<storage, vec4<i32>, read_write> = var undef @binding_point(0, 1)
  %tint_input_attachment_0:ptr<private, vec4<i32>, read_write> = var vec4<i32>(0i)
}

%f = @fragment func(%tint_input_attachment_0_1:vec4<i32> [@color(0)]):void {  # %tint_input_attachment_0_1: 'tint_input_attachment_0'
  $B2: {
    store %tint_input_attachment_0, %tint_input_attachment_0_1
    %5:vec4<i32> = load %tint_input_attachment_0
    store %dce, %5
    ret
  }
}
)";

    std::unordered_map<BindingPoint, uint32_t> map{{BindingPoint{1, 0}, 0}};
    Run(InputAttachment, InputAttachmentConfig{map});

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_InputAttachmentTest, MissingSlotFails) {
    auto* dce = b.Var("dce", ty.ptr(core::AddressSpace::kStorage, ty.vec4f()));
    dce->SetBindingPoint(0, 1);
    b.ir.root_block->Append(dce);
    auto* ia = MakeInputAttachment(b, ty.f32(), 1, 0, 3);

    auto* ep = b.Function("f", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(ep->Block(), [&] {
        b.Store(dce, b.Call(ty.vec4f(), core::BuiltinFn::kInputAttachmentLoad, b.Load(ia)));
        b.Return(ep);
    });

    std::unordered_map<BindingPoint, uint32_t> map{};
    auto result = InputAttachment(mod, InputAttachmentConfig{map});
    ASSERT_NE(result, Success);
    EXPECT_EQ(
        result.Failure().reason,
        "input_attachment used without a corresponding entry in "
        "Options::input_attachment_to_color_index");
}

}  // namespace
}  // namespace tint::msl::writer::raise

// tint-tbdr: end input_attachment-msl
