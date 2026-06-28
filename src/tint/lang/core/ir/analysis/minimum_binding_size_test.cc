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

#include "src/tint/lang/core/ir/analysis/minimum_binding_size.h"

#include <algorithm>
#include <string>

#include "gtest/gtest.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::analysis {
namespace {

struct IR_MinimumBindingSizeAnalysisTest : public IRTestHelper {
    void SetUp() override { mod.properties.Add(Property::kAllowBufferTypes); }
};

TEST_F(IR_MinimumBindingSizeAnalysisTest, FixedSizeStorage) {
    auto* v = b.Var("v", ty.ptr(storage, ty.vec4u()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 16);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, FixedSizeUniform) {
    auto* v = b.Var("v", ty.ptr(storage, ty.vec3u()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 12);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, FixedSizeStorageAndUniform) {
    auto* v1 = b.Var("v1", ty.ptr(storage, ty.vec4u()));
    v1->SetBindingPoint(0, 0);
    mod.root_block->Append(v1);
    auto* v2 = b.Var("v2", ty.ptr(storage, ty.vec3u()));
    v2->SetBindingPoint(0, 1);
    mod.root_block->Append(v2);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 2);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 16);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 12);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, RuntimeArray) {
    auto* v = b.Var("v", ty.ptr(storage, ty.runtime_array(ty.u32())));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 4);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, RuntimeStruct) {
    auto* S =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.u32())},
                                        });
    auto* v = b.Var("v", ty.ptr(storage, S));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 20);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SizedBuffer) {
    auto* v = b.Var("v", ty.ptr(uniform, ty.buffer(200)));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 200);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_FixedSize) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    foo->SetParams({offset});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.vec4u()), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.vec4u()}, v, offset);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 16);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_FixedSize_ConstOffset) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.vec4u()), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.vec4u()}, v, 16_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 32);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_Max) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    foo->SetParams({offset});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.vec4u()), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.vec4u()}, v, offset);
        b.CallExplicit(ty.ptr(storage, ty.u32()), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.u32()}, v, 16_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 20);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_RuntimeArray) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2u())), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.runtime_array(ty.vec2u())}, v, 16_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 24);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_RuntimeStruct) {
    auto* S =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.u32())},
                                        });
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, S), BuiltinFn::kBufferView, Vector<TemplateParameter, 1>{S},
                       v, 16_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 36);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferArrayView_RuntimeArray) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({size});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2u())), BuiltinFn::kBufferArrayView,
                       Vector<TemplateParameter, 1>{ty.runtime_array(ty.vec2u())}, v, 16_u, size);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 24);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferArrayView_RuntimeStruct) {
    auto* S =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.u32())},
                                        });
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({size});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, S), BuiltinFn::kBufferArrayView,
                       Vector<TemplateParameter, 1>{S}, v, 16_u, size);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 36);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferArrayView_RuntimeArray_ConstSize) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2u())), BuiltinFn::kBufferArrayView,
                       Vector<TemplateParameter, 1>{ty.runtime_array(ty.vec2u())}, v, 16_u, 200_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 216);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferView_RuntimeArray_ThroughFunction) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* p = b.FunctionParam("p", ty.ptr(storage, ty.unsized_buffer()));
    foo->SetParams({offset, p});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2u())), BuiltinFn::kBufferView,
                       Vector<TemplateParameter, 1>{ty.runtime_array(ty.vec2u())}, p, offset);
        b.Return(foo);
    });
    auto* bar = b.Function("bar", ty.void_());
    b.Append(bar->Block(), [&] {
        b.Call(ty.void_(), foo, 16_u, v);
        b.Return(bar);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 8);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, BufferArrayView_RuntimeArray_ThroughFunction) {
    auto* v = b.Var("v", ty.ptr(storage, ty.unsized_buffer()));
    v->SetBindingPoint(0, 0);
    mod.root_block->Append(v);

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    auto* p = b.FunctionParam("p", ty.ptr(storage, ty.unsized_buffer()));
    foo->SetParams({offset, size, p});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2u())), BuiltinFn::kBufferArrayView,
                       Vector<TemplateParameter, 1>{ty.runtime_array(ty.vec2u())}, p, offset, size);
        b.Return(foo);
    });
    auto* bar = b.Function("bar", ty.void_());
    b.Append(bar->Block(), [&] {
        b.Call(ty.void_(), foo, 16_u, v, 200_u);
        b.Return(bar);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 1);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 8);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixLoad_ColMajor) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.runtime_array(ty.f32())));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.runtime_array(ty.f16())));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.runtime_array(ty.u32())));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    foo->SetParams({stride});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(left_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{left_ty, Majorness::kColMajor}, left, 0_u,
                       stride);
        b.CallExplicit(right_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{right_ty, Majorness::kColMajor}, right, 0_u,
                       stride);
        b.CallExplicit(result_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{result_ty, Majorness::kColMajor}, result, 0_u,
                       stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 256);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 256);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 256);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixStore_RowMajor) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.runtime_array(ty.f32())));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.runtime_array(ty.f16())));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.runtime_array(ty.u32())));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    auto* l = b.FunctionParam("l", left_ty);
    auto* r = b.FunctionParam("r", right_ty);
    auto* res = b.FunctionParam("res", result_ty);
    foo->SetParams({stride, l, r, res});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, left, 0_u, l, stride);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, right, 0_u, r, stride);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, result, 0_u, res,
                       stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 256);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 256);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 256);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixLoad_ColMajor_ConstOffset) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.runtime_array(ty.f32())));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.runtime_array(ty.f16())));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.runtime_array(ty.u32())));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    foo->SetParams({stride});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(left_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{left_ty, Majorness::kColMajor}, left, 4_u,
                       stride);
        b.CallExplicit(right_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{right_ty, Majorness::kColMajor}, right, 4_u,
                       stride);
        b.CallExplicit(result_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{result_ty, Majorness::kColMajor}, result, 4_u,
                       stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 272);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 264);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 272);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixStore_RowMajor_ConstStride) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.runtime_array(ty.f32())));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.runtime_array(ty.f16())));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.runtime_array(ty.u32())));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* l = b.FunctionParam("l", left_ty);
    auto* r = b.FunctionParam("r", right_ty);
    auto* res = b.FunctionParam("res", result_ty);
    foo->SetParams({l, r, res});
    b.Append(foo->Block(), [&] {
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, left, 0_u, l, 16_u);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, right, 0_u, r, 64_u);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, result, 0_u, res, 64_u);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 480);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 1936);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 3856);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixLoad_ColMajor_ThroughAccess) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.runtime_array(ty.array(ty.f32(), 1024))));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.runtime_array(ty.array(ty.f16(), 1024))));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.runtime_array(ty.array(ty.u32(), 1024))));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    foo->SetParams({stride});
    b.Append(foo->Block(), [&] {
        auto* l_access = b.Access(ty.ptr(storage, ty.array(ty.f32(), 1024)), left, 1_u);
        b.CallExplicit(left_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{left_ty, Majorness::kColMajor}, l_access, 0_u,
                       stride);

        auto* r_access = b.Access(ty.ptr(storage, ty.array(ty.f16(), 1024)), right, 1_u);
        b.CallExplicit(right_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{right_ty, Majorness::kColMajor}, r_access, 0_u,
                       stride);

        auto* res_access = b.Access(ty.ptr(storage, ty.array(ty.u32(), 1024)), result, 1_u);
        b.CallExplicit(result_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{result_ty, Majorness::kColMajor}, res_access,
                       0_u, stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 256 + 4096);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 256 + 2048);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 256 + 4096);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixStore_RowMajor_ThroughAccess) {
    auto* S_f32 =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.f32())},
                                        });
    auto* S_f16 =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.f16())},
                                        });
    auto* S_u8 =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.vec4u()},
                                            {mod.symbols.New("b"), ty.runtime_array(ty.u32())},
                                        });

    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, S_f32));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, S_f16));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, S_u8));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    auto* l = b.FunctionParam("l", left_ty);
    auto* r = b.FunctionParam("r", right_ty);
    auto* res = b.FunctionParam("res", result_ty);
    foo->SetParams({stride, l, r, res});
    b.Append(foo->Block(), [&] {
        auto* l_access = b.Access(ty.ptr(storage, ty.runtime_array(ty.f32())), left, 1_u);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, l_access, 0_u, l,
                       stride);

        auto* r_access = b.Access(ty.ptr(storage, ty.runtime_array(ty.f16())), right, 1_u);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, r_access, 0_u, r,
                       stride);

        auto* res_access = b.Access(ty.ptr(storage, ty.runtime_array(ty.u32())), result, 1_u);
        b.CallExplicit(ty.void_(), BuiltinFn::kSubgroupMatrixStore,
                       Vector<TemplateParameter, 1>{Majorness::kRowMajor}, res_access, 0_u, res,
                       stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 272);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 272);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 272);
}

TEST_F(IR_MinimumBindingSizeAnalysisTest, SubgroupMatrixLoad_ColMajor_ThroughBufferView) {
    auto* left_ty = ty.subgroup_matrix_left(ty.f32(), 8, 8);
    auto* right_ty = ty.subgroup_matrix_right(ty.f16(), 8, 16);
    auto* result_ty = ty.subgroup_matrix_result(ty.u8(), 16, 16);

    auto* left = b.Var("left", ty.ptr(storage, ty.unsized_buffer()));
    left->SetBindingPoint(0, 0);
    mod.root_block->Append(left);
    auto* right = b.Var("right", ty.ptr(storage, ty.unsized_buffer()));
    right->SetBindingPoint(0, 1);
    mod.root_block->Append(right);
    auto* result = b.Var("result", ty.ptr(storage, ty.unsized_buffer()));
    result->SetBindingPoint(0, 2);
    mod.root_block->Append(result);

    auto* foo = b.Function("foo", ty.void_());
    auto* stride = b.FunctionParam("stride", ty.u32());
    foo->SetParams({stride});
    b.Append(foo->Block(), [&] {
        auto* l_access =
            b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.f32())), BuiltinFn::kBufferView,
                           Vector<TemplateParameter, 1>{ty.runtime_array(ty.f32())}, left, 4096_u);
        auto* l_let = b.Let("l_let", l_access);
        b.CallExplicit(left_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{left_ty, Majorness::kColMajor}, l_let, 0_u,
                       stride);

        auto* r_access =
            b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.f16())), BuiltinFn::kBufferView,
                           Vector<TemplateParameter, 1>{ty.runtime_array(ty.f16())}, right, 4096_u);
        auto* r_let = b.Let("r_let", r_access);
        b.CallExplicit(right_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{right_ty, Majorness::kColMajor}, r_let, 0_u,
                       stride);

        auto* res_access = b.CallExplicit(
            ty.ptr(storage, ty.runtime_array(ty.u32())), BuiltinFn::kBufferView,
            Vector<TemplateParameter, 1>{ty.runtime_array(ty.u32())}, result, 4096_u);
        auto* res_let = b.Let("res_let", res_access);
        b.CallExplicit(result_ty, BuiltinFn::kSubgroupMatrixLoad,
                       Vector<TemplateParameter, 2>{result_ty, Majorness::kColMajor}, res_let, 0_u,
                       stride);
        b.Return(foo);
    });

    auto info = MinimumBindingSize(mod);
    EXPECT_EQ(info.binding_sizes.size(), 3);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 0})], 256 + 4096);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 1})], 256 + 4096);
    EXPECT_EQ(info.binding_sizes[(BindingPoint{0, 2})], 256 + 4096);
}

}  // namespace
}  // namespace tint::core::ir::analysis
