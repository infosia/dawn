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

#include "src/tint/lang/msl/writer/raise/decompose_buffer.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"

namespace tint::msl::writer::raise {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class MslWriter_DecomposeBufferTest : public core::ir::transform::TransformTest {
    void SetUp() override {
        capabilities.Add(core::ir::Capability::kMslAllowEntryPointInterface,
                         core::ir::Capability::kAllow8BitIntegers);
    }
};

TEST_F(MslWriter_DecomposeBufferTest, BufferView_u32_ZeroOffset) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.u32()), core::BuiltinFn::kBufferView,
                                    Vector{ty.u32()}, gv, 0_u);
        b.Load(view);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = bufferView<u32> %gv, 0u
    %4:u32 = load %3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = msl.pointer_offset<u32> %gv, 0u
    %4:u32 = load %3
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferArrayView_ArrayU32) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* func = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    func->SetParams({offset, size});
    b.Append(func->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                    core::BuiltinFn::kBufferArrayView,
                                    Vector{ty.runtime_array(ty.u32())}, gv, offset, size);
        b.Load(b.Access(ty.ptr(storage, ty.u32()), view, 4_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func(%offset:u32, %size:u32):void {
  $B2: {
    %5:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv, %offset, %size
    %6:ptr<storage, u32, read_write> = access %5, 4u
    %7:u32 = load %6
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func(%offset:u32, %size:u32):void {
  $B2: {
    %5:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %offset
    %6:ptr<storage, u32, read_write> = access %5, 4u
    %7:u32 = load %6
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferView_ArrayU32_ArrayLength) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                    core::BuiltinFn::kBufferView,
                                    Vector{ty.runtime_array(ty.u32())}, gv, 16_u);
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, view);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, array<u32>, read_write> = bufferView<array<u32>> %gv, 16u
    %4:u32 = arrayLength %3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %4:u32 = arrayLength %3
    %5:u32 = sub %4, 16u
    %6:u32 = div %5, 4u
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferArrayView_ArrayU32_ThroughFunctionToArrayLength) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* bar = b.Function("bar", ty.void_());
    auto* buffer = b.FunctionParam("buffer", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({buffer});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, buffer);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({offset, size});
    b.Append(foo->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                    core::BuiltinFn::kBufferArrayView,
                                    Vector{ty.runtime_array(ty.u32())}, gv, offset, size);
        b.Call(bar, view);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %4:u32 = arrayLength %buffer
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %8:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv, %offset, %size
    %9:void = call %bar, %8
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  size:u32 @offset(4)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = arrayLength %4
    %8:bool = eq %6, 0u
    %9:u32 = select %6, %7, %8
    %10:u32 = sub %9, %5
    %11:u32 = div %10, 4u
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %15:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %offset
    %16:buffer_bundle_0 = construct %15, %offset, %size
    %17:void = call %bar, %16
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferArrayView_RuntimeStruct_ThroughFunctionToArrayLength) {
    auto* S = ty.Struct(mod.symbols.Register("S"),
                        {
                            {mod.symbols.Register("a"), ty.vec4(ty.u32())},
                            {mod.symbols.Register("b"), ty.runtime_array(ty.u32())},
                        });
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* bar = b.Function("bar", ty.void_());
    auto* buffer = b.FunctionParam("buffer", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({buffer});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, buffer);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({offset, size});
    b.Append(foo->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, S), core::BuiltinFn::kBufferArrayView,
                                    Vector{S}, gv, offset, size);
        auto* access = b.Access(ty.ptr(storage, ty.runtime_array(ty.u32())), view, 1_u);
        b.Call(bar, access);
        b.Return(foo);
    });

    auto* src = R"(
S = struct @align(16) {
  a:vec4<u32> @offset(0)
  b:array<u32> @offset(16)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %4:u32 = arrayLength %buffer
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %8:ptr<storage, S, read_write> = bufferArrayView<S> %gv, %offset, %size
    %9:ptr<storage, array<u32>, read_write> = access %8, 1u
    %10:void = call %bar, %9
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(16) {
  a:vec4<u32> @offset(0)
  b:array<u32> @offset(16)
}

buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  size:u32 @offset(4)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = arrayLength %4
    %8:bool = eq %6, 0u
    %9:u32 = select %6, %7, %8
    %10:u32 = sub %9, %5
    %11:u32 = div %10, 4u
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %15:ptr<storage, S, read_write> = msl.pointer_offset<S> %gv, %offset
    %16:ptr<storage, array<u32>, read_write> = access %15, 1u
    %17:u32 = add %offset, 16u
    %18:buffer_bundle_0 = construct %16, %17, %size
    %19:void = call %bar, %18
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest,
       BufferArrayView_RuntimeStruct_ThroughFunctionToArrayLength_MultiSource) {
    auto* S = ty.Struct(mod.symbols.Register("S"),
                        {
                            {mod.symbols.Register("a"), ty.vec4(ty.u32())},
                            {mod.symbols.Register("b"), ty.runtime_array(ty.u32())},
                        });
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);
    auto* arr = b.Var("arr", ty.ptr(storage, ty.runtime_array(ty.u32())));
    arr->SetBindingPoint(0, 1);
    mod.root_block->Append(arr);

    auto* bar = b.Function("bar", ty.void_());
    auto* buffer = b.FunctionParam("buffer", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({buffer});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, buffer);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({offset, size});
    b.Append(foo->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, S), core::BuiltinFn::kBufferArrayView,
                                    Vector{S}, gv, offset, size);
        auto* access = b.Access(ty.ptr(storage, ty.runtime_array(ty.u32())), view, 1_u);
        b.Call(bar, access);
        b.Return(foo);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    b.Append(foobar->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                    core::BuiltinFn::kBufferView,
                                    Vector{ty.runtime_array(ty.u32())}, gv, 16_u);
        b.Call(bar, view);
        b.Return(foobar);
    });

    auto* baz = b.Function("baz", ty.void_());
    b.Append(baz->Block(), [&] {
        b.Call(bar, arr);
        b.Return(baz);
    });

    auto* src = R"(
S = struct @align(16) {
  a:vec4<u32> @offset(0)
  b:array<u32> @offset(16)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %arr:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 1)
}

%bar = func(%buffer:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %5:u32 = arrayLength %buffer
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %9:ptr<storage, S, read_write> = bufferArrayView<S> %gv, %offset, %size
    %10:ptr<storage, array<u32>, read_write> = access %9, 1u
    %11:void = call %bar, %10
    ret
  }
}
%foobar = func():void {
  $B4: {
    %13:ptr<storage, array<u32>, read_write> = bufferView<array<u32>> %gv, 16u
    %14:void = call %bar, %13
    ret
  }
}
%baz = func():void {
  $B5: {
    %16:void = call %bar, %arr
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(16) {
  a:vec4<u32> @offset(0)
  b:array<u32> @offset(16)
}

buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  size:u32 @offset(4)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %arr:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 1)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %5:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %6:u32 = access %buffer, 1u
    %7:u32 = access %buffer, 2u
    %8:u32 = arrayLength %5
    %9:bool = eq %7, 0u
    %10:u32 = select %7, %8, %9
    %11:u32 = sub %10, %6
    %12:u32 = div %11, 4u
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %16:ptr<storage, S, read_write> = msl.pointer_offset<S> %gv, %offset
    %17:ptr<storage, array<u32>, read_write> = access %16, 1u
    %18:buffer_bundle_0 = construct %17, 0u, 0u
    %19:void = call %bar, %18
    ret
  }
}
%foobar = func():void {
  $B4: {
    %21:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %22:buffer_bundle_0 = construct %21, 16u, 0u
    %23:void = call %bar, %22
    ret
  }
}
%baz = func():void {
  $B5: {
    %25:buffer_bundle_0 = construct %arr, 0u, 0u
    %26:void = call %bar, %25
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest,
       BufferArrayView_ArrayU32_ThroughMultipleFunctionToArrayLength) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* bar = b.Function("bar", ty.void_());
    auto* buffer = b.FunctionParam("buffer", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({buffer});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, buffer);
        b.Return(bar);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    auto* foobar_p = b.FunctionParam("foobar_p", ty.ptr(storage, ty.runtime_array(ty.u32())));
    foobar->SetParams({foobar_p});
    b.Append(foobar->Block(), [&] {
        b.Call(bar, foobar_p);
        b.Return(foobar);
    });

    auto* baz = b.Function("baz", ty.void_());
    auto* baz_p = b.FunctionParam("baz_p", ty.ptr(storage, ty.runtime_array(ty.u32())));
    baz->SetParams({baz_p});
    b.Append(baz->Block(), [&] {
        b.Call(foobar, baz_p);
        b.Return(baz);
    });

    auto* foo = b.Function("foo", ty.void_());
    auto* offset = b.FunctionParam("offset", ty.u32());
    auto* size = b.FunctionParam("size", ty.u32());
    foo->SetParams({offset, size});
    b.Append(foo->Block(), [&] {
        auto* view = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                    core::BuiltinFn::kBufferArrayView,
                                    Vector{ty.runtime_array(ty.u32())}, gv, offset, size);
        b.Call(baz, view);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %4:u32 = arrayLength %buffer
    ret
  }
}
%foobar = func(%foobar_p:ptr<storage, array<u32>, read_write>):void {
  $B3: {
    %7:void = call %bar, %foobar_p
    ret
  }
}
%baz = func(%baz_p:ptr<storage, array<u32>, read_write>):void {
  $B4: {
    %10:void = call %foobar, %baz_p
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B5: {
    %14:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv, %offset, %size
    %15:void = call %baz, %14
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  size:u32 @offset(4)
}

$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = arrayLength %4
    %8:bool = eq %6, 0u
    %9:u32 = select %6, %7, %8
    %10:u32 = sub %9, %5
    %11:u32 = div %10, 4u
    ret
  }
}
%foobar = func(%foobar_p:buffer_bundle_0):void {
  $B3: {
    %14:ptr<storage, array<u32>, read_write> = access %foobar_p, 0u
    %15:u32 = access %foobar_p, 1u
    %16:u32 = access %foobar_p, 2u
    %17:buffer_bundle_0 = construct %14, %15, %16
    %18:void = call %bar, %17
    ret
  }
}
%baz = func(%baz_p:buffer_bundle_0):void {
  $B4: {
    %21:ptr<storage, array<u32>, read_write> = access %baz_p, 0u
    %22:u32 = access %baz_p, 1u
    %23:u32 = access %baz_p, 2u
    %24:buffer_bundle_0 = construct %21, %22, %23
    %25:void = call %foobar, %24
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B5: {
    %29:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %offset
    %30:buffer_bundle_0 = construct %29, %offset, %size
    %31:void = call %baz, %30
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferLength_FromOperand) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.buffer(64)));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* call = b.Call(ty.u32(), core::BuiltinFn::kBufferLength, gv, 16_u);
        b.Let("len", call);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer<64>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:u32 = bufferLength %gv, 16u
    %len:u32 = let %3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer<64>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %len:u32 = let 16u
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferLength_FromType) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.buffer(64)));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* call = b.Call(ty.u32(), core::BuiltinFn::kBufferLength, gv);
        b.Let("len", call);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer<64>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:u32 = bufferLength %gv
    %len:u32 = let %3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer<64>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %len:u32 = let 64u
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferLength_FromArrayLength) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* call = b.Call(ty.u32(), core::BuiltinFn::kBufferLength, gv);
        b.Let("len", call);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:u32 = bufferLength %gv
    %len:u32 = let %3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, array<u8>, read_write> = msl.pointer_offset<array<u8>> %gv, 0u
    %4:u32 = arrayLength %3
    %len:u32 = let %4
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, MultipleCallsFromVariable) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* c1 = b.CallExplicit(ty.ptr(storage, ty.u32()), core::BuiltinFn::kBufferView,
                                  Vector{ty.u32()}, gv, 0_u);
        b.Load(c1);
        auto* c2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, gv, 16_u, 32_u);
        b.Load(b.Access(ty.ptr(storage, ty.u32()), c2, 1_u));
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = bufferView<u32> %gv, 0u
    %4:u32 = load %3
    %5:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv, 16u, 32u
    %6:ptr<storage, u32, read_write> = access %5, 1u
    %7:u32 = load %6
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = msl.pointer_offset<u32> %gv, 0u
    %4:u32 = load %3
    %5:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %6:ptr<storage, u32, read_write> = access %5, 1u
    %7:u32 = load %6
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, RemoveLets) {
    auto* gv = b.Var("gv", ty.ptr(storage, ty.unsized_buffer()));
    gv->SetBindingPoint(0, 0);
    mod.root_block->Append(gv);

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* l1 = b.Let("l1", gv);
        auto* c1 = b.CallExplicit(ty.ptr(storage, ty.u32()), core::BuiltinFn::kBufferView,
                                  Vector{ty.u32()}, l1, 0_u);
        auto* l2 = b.Let("l2", c1);
        b.Load(l2);
        auto* l3 = b.Let("l3", gv);
        auto* l4 = b.Let("l4", l3);
        auto* l5 = b.Let("l5", l4);
        auto* c2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, l5, 16_u, 32_u);
        b.Load(b.Access(ty.ptr(storage, ty.u32()), c2, 1_u));
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %l1:ptr<storage, buffer, read_write> = let %gv
    %4:ptr<storage, u32, read_write> = bufferView<u32> %l1, 0u
    %l2:ptr<storage, u32, read_write> = let %4
    %6:u32 = load %l2
    %l3:ptr<storage, buffer, read_write> = let %gv
    %l4:ptr<storage, buffer, read_write> = let %l3
    %l5:ptr<storage, buffer, read_write> = let %l4
    %10:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %l5, 16u, 32u
    %11:ptr<storage, u32, read_write> = access %10, 1u
    %12:u32 = load %11
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = msl.pointer_offset<u32> %gv, 0u
    %4:u32 = load %3
    %5:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %6:ptr<storage, u32, read_write> = access %5, 1u
    %7:u32 = load %6
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::msl::writer::raise
