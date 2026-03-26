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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
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
    auto* offset = b.FunctionParam("offset", ty.i32());
    auto* size = b.FunctionParam("size", ty.i32());
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

%foo = func(%offset:i32, %size:i32):void {
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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%foo = func(%offset:i32, %size:i32):void {
  $B2: {
    %5:u32 = bitcast<u32> %offset
    %6:u32 = bitcast<u32> %size
    %7:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %5
    %8:ptr<storage, u32, read_write> = access %7, 4u
    %9:u32 = load %8
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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %4:u32 = arrayLength %3
    %5:bool = eq 0u, 0u
    %6:u32 = select 0u, %4, %5
    %7:bool = eq 0u, 0u
    %8:u32 = select 0u, 16u, %7
    %9:u32 = sub %6, %8
    %10:u32 = sub %9, 0u
    %11:u32 = div %10, 4u
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
    auto* offset = b.FunctionParam("offset", ty.i32());
    auto* size = b.FunctionParam("size", ty.i32());
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
%foo = func(%offset:i32, %size:i32):void {
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
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = access %buffer, 3u
    %8:u32 = access %buffer, 4u
    %9:u32 = arrayLength %4
    %10:bool = eq %8, 0u
    %11:u32 = select %8, %9, %10
    %12:bool = eq %7, 0u
    %13:u32 = select %7, %11, %12
    %14:u32 = select 0u, %5, %12
    %15:u32 = sub %13, %14
    %16:u32 = sub %15, %6
    %17:u32 = div %16, 4u
    ret
  }
}
%foo = func(%offset:i32, %size:i32):void {
  $B3: {
    %21:u32 = bitcast<u32> %offset
    %22:u32 = bitcast<u32> %size
    %23:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %21
    %24:buffer_bundle_0 = construct %23, %21, 0u, %22, 0u
    %25:void = call %bar, %24
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
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = access %buffer, 3u
    %8:u32 = access %buffer, 4u
    %9:u32 = arrayLength %4
    %10:bool = eq %8, 0u
    %11:u32 = select %8, %9, %10
    %12:bool = eq %7, 0u
    %13:u32 = select %7, %11, %12
    %14:u32 = select 0u, %5, %12
    %15:u32 = sub %13, %14
    %16:u32 = sub %15, %6
    %17:u32 = div %16, 4u
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %21:ptr<storage, S, read_write> = msl.pointer_offset<S> %gv, %offset
    %22:ptr<storage, array<u32>, read_write> = access %21, 1u
    %23:buffer_bundle_0 = construct %22, %offset, 16u, %size, 0u
    %24:void = call %bar, %23
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
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
  %arr:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 1)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %5:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %6:u32 = access %buffer, 1u
    %7:u32 = access %buffer, 2u
    %8:u32 = access %buffer, 3u
    %9:u32 = access %buffer, 4u
    %10:u32 = arrayLength %5
    %11:bool = eq %9, 0u
    %12:u32 = select %9, %10, %11
    %13:bool = eq %8, 0u
    %14:u32 = select %8, %12, %13
    %15:u32 = select 0u, %6, %13
    %16:u32 = sub %14, %15
    %17:u32 = sub %16, %7
    %18:u32 = div %17, 4u
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B3: {
    %22:ptr<storage, S, read_write> = msl.pointer_offset<S> %gv, %offset
    %23:ptr<storage, array<u32>, read_write> = access %22, 1u
    %24:buffer_bundle_0 = construct %23, %offset, 16u, %size, 0u
    %25:void = call %bar, %24
    ret
  }
}
%foobar = func():void {
  $B4: {
    %27:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, 16u
    %28:buffer_bundle_0 = construct %27, 16u, 0u, 0u, 0u
    %29:void = call %bar, %28
    ret
  }
}
%baz = func():void {
  $B5: {
    %31:buffer_bundle_0 = construct %arr, 0u, 0u, 0u, 0u
    %32:void = call %bar, %31
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
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%buffer:buffer_bundle_0):void {
  $B2: {
    %4:ptr<storage, array<u32>, read_write> = access %buffer, 0u
    %5:u32 = access %buffer, 1u
    %6:u32 = access %buffer, 2u
    %7:u32 = access %buffer, 3u
    %8:u32 = access %buffer, 4u
    %9:u32 = arrayLength %4
    %10:bool = eq %8, 0u
    %11:u32 = select %8, %9, %10
    %12:bool = eq %7, 0u
    %13:u32 = select %7, %11, %12
    %14:u32 = select 0u, %5, %12
    %15:u32 = sub %13, %14
    %16:u32 = sub %15, %6
    %17:u32 = div %16, 4u
    ret
  }
}
%foobar = func(%foobar_p:buffer_bundle_0):void {
  $B3: {
    %20:ptr<storage, array<u32>, read_write> = access %foobar_p, 0u
    %21:u32 = access %foobar_p, 1u
    %22:u32 = access %foobar_p, 2u
    %23:u32 = access %foobar_p, 3u
    %24:u32 = access %foobar_p, 4u
    %25:buffer_bundle_0 = construct %20, %21, %22, %23, %24
    %26:void = call %bar, %25
    ret
  }
}
%baz = func(%baz_p:buffer_bundle_0):void {
  $B4: {
    %29:ptr<storage, array<u32>, read_write> = access %baz_p, 0u
    %30:u32 = access %baz_p, 1u
    %31:u32 = access %baz_p, 2u
    %32:u32 = access %baz_p, 3u
    %33:u32 = access %baz_p, 4u
    %34:buffer_bundle_0 = construct %29, %30, %31, %32, %33
    %35:void = call %foobar, %34
    ret
  }
}
%foo = func(%offset:u32, %size:u32):void {
  $B5: {
    %39:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv, %offset
    %40:buffer_bundle_0 = construct %39, %offset, 0u, %size, 0u
    %41:void = call %baz, %40
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
  %gv:ptr<storage, array<u8, 64>, read_write> = var undef @binding_point(0, 0)
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
  %gv:ptr<storage, array<u8, 64>, read_write> = var undef @binding_point(0, 0)
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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%foo = func():void {
  $B2: {
    %3:u32 = arrayLength %gv
    %len:u32 = let %3
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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
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
  %gv:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
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

TEST_F(MslWriter_DecomposeBufferTest, UnifyParameters) {
    auto* large = b.Var("large", ty.ptr(storage, ty.buffer(1024)));
    large->SetBindingPoint(0, 0);
    mod.root_block->Append(large);
    auto* small = b.Var("small", ty.ptr(workgroup, ty.buffer(512)));
    mod.root_block->Append(small);

    auto* baz = b.Function("baz", ty.void_());
    auto* baz_p1 = b.FunctionParam("baz_p1", ty.ptr(storage, ty.unsized_buffer()));
    auto* baz_p2 = b.FunctionParam("baz_p2", ty.ptr(workgroup, ty.unsized_buffer()));
    baz->SetParams({baz_p1, baz_p2});
    b.Append(baz->Block(), [&] { b.Return(baz); });

    auto* bar = b.Function("bar", ty.void_());
    auto* bar_p1 = b.FunctionParam("bar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* bar_p2 = b.FunctionParam("bar_p2", ty.ptr(workgroup, ty.buffer(256)));
    bar->SetParams({bar_p1, bar_p2});
    b.Append(bar->Block(), [&] {
        b.Call(ty.void_(), baz, bar_p1, bar_p2);
        b.Return(bar);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    auto* foobar_p1 = b.FunctionParam("foobar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* foobar_p2 = b.FunctionParam("foobar_p2", ty.ptr(workgroup, ty.buffer(256)));
    foobar->SetParams({foobar_p1, foobar_p2});
    b.Append(foobar->Block(), [&] {
        b.Call(ty.void_(), bar, foobar_p1, foobar_p2);
        b.Return(foobar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.Call(ty.void_(), foobar, large, small);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %large:ptr<storage, buffer<1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, buffer<512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, buffer, read_write>, %baz_p2:ptr<workgroup, buffer, read_write>):void {
  $B2: {
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, buffer<512>, read_write>, %bar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B3: {
    %9:void = call %baz, %bar_p1, %bar_p2
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, buffer<512>, read_write>, %foobar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B4: {
    %13:void = call %bar, %foobar_p1, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %15:void = call %foobar, %large, %small
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %large:ptr<storage, array<u8, 1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, array<u8, 512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, array<u8>, read_write>, %baz_p2:ptr<workgroup, array<u8>, read_write>):void {
  $B2: {
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, array<u8, 512>, read_write>, %bar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B3: {
    %9:ptr<storage, array<u8>, read_write> = msl.pointer_offset<array<u8>> %bar_p1, 0u
    %10:ptr<workgroup, array<u8>, read_write> = msl.pointer_offset<array<u8>> %bar_p2, 0u
    %11:void = call %baz, %9, %10
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, array<u8, 512>, read_write>, %foobar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B4: {
    %15:void = call %bar, %foobar_p1, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %17:ptr<storage, array<u8, 512>, read_write> = msl.pointer_offset<array<u8, 512>> %large, 0u
    %18:ptr<workgroup, array<u8, 256>, read_write> = msl.pointer_offset<array<u8, 256>> %small, 0u
    %19:void = call %foobar, %17, %18
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, UnifyParameters_Lets) {
    auto* large = b.Var("large", ty.ptr(storage, ty.buffer(1024)));
    large->SetBindingPoint(0, 0);
    mod.root_block->Append(large);
    auto* small = b.Var("small", ty.ptr(workgroup, ty.buffer(512)));
    mod.root_block->Append(small);

    auto* baz = b.Function("baz", ty.void_());
    auto* baz_p1 = b.FunctionParam("baz_p1", ty.ptr(storage, ty.unsized_buffer()));
    auto* baz_p2 = b.FunctionParam("baz_p2", ty.ptr(workgroup, ty.unsized_buffer()));
    baz->SetParams({baz_p1, baz_p2});
    b.Append(baz->Block(), [&] { b.Return(baz); });

    auto* bar = b.Function("bar", ty.void_());
    auto* bar_p1 = b.FunctionParam("bar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* bar_p2 = b.FunctionParam("bar_p2", ty.ptr(workgroup, ty.buffer(256)));
    bar->SetParams({bar_p1, bar_p2});
    b.Append(bar->Block(), [&] {
        auto* l1 = b.Let("l1", bar_p1);
        auto* l2 = b.Let("l2", bar_p2);
        b.Call(ty.void_(), baz, l1, l2);
        b.Return(bar);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    auto* foobar_p1 = b.FunctionParam("foobar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* foobar_p2 = b.FunctionParam("foobar_p2", ty.ptr(workgroup, ty.buffer(256)));
    foobar->SetParams({foobar_p1, foobar_p2});
    b.Append(foobar->Block(), [&] {
        auto* l3 = b.Let("l3", foobar_p1);
        b.Call(ty.void_(), bar, l3, foobar_p2);
        b.Return(foobar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.Call(ty.void_(), foobar, large, small);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %large:ptr<storage, buffer<1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, buffer<512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, buffer, read_write>, %baz_p2:ptr<workgroup, buffer, read_write>):void {
  $B2: {
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, buffer<512>, read_write>, %bar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B3: {
    %l1:ptr<storage, buffer<512>, read_write> = let %bar_p1
    %l2:ptr<workgroup, buffer<256>, read_write> = let %bar_p2
    %11:void = call %baz, %l1, %l2
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, buffer<512>, read_write>, %foobar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B4: {
    %l3:ptr<storage, buffer<512>, read_write> = let %foobar_p1
    %16:void = call %bar, %l3, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %18:void = call %foobar, %large, %small
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %large:ptr<storage, array<u8, 1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, array<u8, 512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, array<u8>, read_write>, %baz_p2:ptr<workgroup, array<u8>, read_write>):void {
  $B2: {
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, array<u8, 512>, read_write>, %bar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B3: {
    %9:ptr<storage, array<u8>, read_write> = msl.pointer_offset<array<u8>> %bar_p1, 0u
    %10:ptr<workgroup, array<u8>, read_write> = msl.pointer_offset<array<u8>> %bar_p2, 0u
    %11:void = call %baz, %9, %10
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, array<u8, 512>, read_write>, %foobar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B4: {
    %15:void = call %bar, %foobar_p1, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %17:ptr<storage, array<u8, 512>, read_write> = msl.pointer_offset<array<u8, 512>> %large, 0u
    %18:ptr<workgroup, array<u8, 256>, read_write> = msl.pointer_offset<array<u8, 256>> %small, 0u
    %19:void = call %foobar, %17, %18
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, UnifyParameters_Lets_Unused) {
    auto* baz = b.Function("baz", ty.void_());
    auto* baz_p1 = b.FunctionParam("baz_p1", ty.ptr(storage, ty.unsized_buffer()));
    auto* baz_p2 = b.FunctionParam("baz_p2", ty.ptr(workgroup, ty.unsized_buffer()));
    baz->SetParams({baz_p1, baz_p2});
    b.Append(baz->Block(), [&] {
        b.Let("l1", baz_p1);
        b.Let("l2", baz_p2);
        b.Return(baz);
    });

    auto* src = R"(
%baz = func(%baz_p1:ptr<storage, buffer, read_write>, %baz_p2:ptr<workgroup, buffer, read_write>):void {
  $B1: {
    %l1:ptr<storage, buffer, read_write> = let %baz_p1
    %l2:ptr<workgroup, buffer, read_write> = let %baz_p2
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
%baz = func(%baz_p1:ptr<storage, array<u8>, read_write>, %baz_p2:ptr<workgroup, array<u8>, read_write>):void {
  $B1: {
    %l1:ptr<storage, array<u8>, read_write> = let %baz_p1
    %l2:ptr<workgroup, array<u8>, read_write> = let %baz_p2
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, UnifyParameters_WithUses) {
    auto* large = b.Var("large", ty.ptr(storage, ty.buffer(1024)));
    large->SetBindingPoint(0, 0);
    mod.root_block->Append(large);
    auto* small = b.Var("small", ty.ptr(workgroup, ty.buffer(512)));
    mod.root_block->Append(small);

    auto* baz = b.Function("baz", ty.void_());
    auto* baz_p1 = b.FunctionParam("baz_p1", ty.ptr(storage, ty.array(ty.u32(), 64)));
    auto* baz_p2 = b.FunctionParam("baz_p2", ty.ptr(workgroup, ty.runtime_array(ty.u32())));
    baz->SetParams({baz_p1, baz_p2});
    b.Append(baz->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, baz_p2);
        b.Return(baz);
    });

    auto* bar = b.Function("bar", ty.void_());
    auto* bar_p1 = b.FunctionParam("bar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* bar_p2 = b.FunctionParam("bar_p2", ty.ptr(workgroup, ty.buffer(256)));
    bar->SetParams({bar_p1, bar_p2});
    b.Append(bar->Block(), [&] {
        auto* v1 =
            b.CallExplicit(ty.ptr(storage, ty.array(ty.u32(), 64)), core::BuiltinFn::kBufferView,
                           Vector{ty.array(ty.u32(), 64)}, bar_p1, 32_u);
        auto* v2 = b.CallExplicit(ty.ptr(workgroup, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, bar_p2, 4_u, 12_u);
        b.Call(ty.void_(), baz, v1, v2);
        b.Return(bar);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    auto* foobar_p1 = b.FunctionParam("foobar_p1", ty.ptr(storage, ty.buffer(512)));
    auto* foobar_p2 = b.FunctionParam("foobar_p2", ty.ptr(workgroup, ty.buffer(256)));
    foobar->SetParams({foobar_p1, foobar_p2});
    b.Append(foobar->Block(), [&] {
        b.Call(ty.void_(), bar, foobar_p1, foobar_p2);
        b.Return(foobar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.Call(ty.void_(), foobar, large, small);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %large:ptr<storage, buffer<1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, buffer<512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, array<u32, 64>, read_write>, %baz_p2:ptr<workgroup, array<u32>, read_write>):void {
  $B2: {
    %6:u32 = arrayLength %baz_p2
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, buffer<512>, read_write>, %bar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B3: {
    %10:ptr<storage, array<u32, 64>, read_write> = bufferView<array<u32, 64>> %bar_p1, 32u
    %11:ptr<workgroup, array<u32>, read_write> = bufferArrayView<array<u32>> %bar_p2, 4u, 12u
    %12:void = call %baz, %10, %11
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, buffer<512>, read_write>, %foobar_p2:ptr<workgroup, buffer<256>, read_write>):void {
  $B4: {
    %16:void = call %bar, %foobar_p1, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %18:void = call %foobar, %large, %small
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<workgroup, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %large:ptr<storage, array<u8, 1024>, read_write> = var undef @binding_point(0, 0)
  %small:ptr<workgroup, array<u8, 512>, read_write> = var undef
}

%baz = func(%baz_p1:ptr<storage, array<u32, 64>, read_write>, %baz_p2:buffer_bundle_0):void {
  $B2: {
    %6:ptr<workgroup, array<u32>, read_write> = access %baz_p2, 0u
    %7:u32 = access %baz_p2, 1u
    %8:u32 = access %baz_p2, 2u
    %9:u32 = access %baz_p2, 3u
    %10:u32 = access %baz_p2, 4u
    %11:bool = eq %9, 0u
    %12:u32 = select %9, %10, %11
    %13:u32 = select 0u, %7, %11
    %14:u32 = sub %12, %13
    %15:u32 = sub %14, %8
    %16:u32 = div %15, 4u
    ret
  }
}
%bar = func(%bar_p1:ptr<storage, array<u8, 512>, read_write>, %bar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B3: {
    %20:ptr<storage, array<u32, 64>, read_write> = msl.pointer_offset<array<u32, 64>> %bar_p1, 32u
    %21:ptr<workgroup, array<u32>, read_write> = msl.pointer_offset<array<u32>> %bar_p2, 4u
    %22:buffer_bundle_0 = construct %21, 4u, 0u, 12u, 0u
    %23:void = call %baz, %20, %22
    ret
  }
}
%foobar = func(%foobar_p1:ptr<storage, array<u8, 512>, read_write>, %foobar_p2:ptr<workgroup, array<u8, 256>, read_write>):void {
  $B4: {
    %27:void = call %bar, %foobar_p1, %foobar_p2
    ret
  }
}
%foo = func():void {
  $B5: {
    %29:ptr<storage, array<u8, 512>, read_write> = msl.pointer_offset<array<u8, 512>> %large, 0u
    %30:ptr<workgroup, array<u8, 256>, read_write> = msl.pointer_offset<array<u8, 256>> %small, 0u
    %31:void = call %foobar, %29, %30
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, FunctionCall_MultiCallSite_SameCaller) {
    auto* gv1 = b.Var("gv1", ty.ptr(storage, ty.unsized_buffer()));
    gv1->SetBindingPoint(0, 0);
    mod.root_block->Append(gv1);
    auto* gv2 = b.Var("gv2", ty.ptr(storage, ty.buffer(128)));
    gv2->SetBindingPoint(0, 0);
    mod.root_block->Append(gv2);
    auto* gv3 = b.Var("gv3", ty.ptr(storage, ty.runtime_array(ty.u32())));
    gv3->SetBindingPoint(0, 0);
    mod.root_block->Append(gv3);

    auto* bar = b.Function("bar", ty.void_());
    auto* param = b.FunctionParam("param", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({param});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, param);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* v1 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, gv1, 16_u, 64_u);
        b.Call(ty.void_(), bar, v1);
        auto* v2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferView, Vector{ty.runtime_array(ty.u32())},
                                  gv2, 32_u);
        b.Call(ty.void_(), bar, v2);
        b.Call(ty.void_(), bar, gv3);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv1:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, buffer<128>, read_write> = var undef @binding_point(0, 0)
  %gv3:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%param:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %6:u32 = arrayLength %param
    ret
  }
}
%foo = func():void {
  $B3: {
    %8:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv1, 16u, 64u
    %9:void = call %bar, %8
    %10:ptr<storage, array<u32>, read_write> = bufferView<array<u32>> %gv2, 32u
    %11:void = call %bar, %10
    %12:void = call %bar, %gv3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv1:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, array<u8, 128>, read_write> = var undef @binding_point(0, 0)
  %gv3:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%param:buffer_bundle_0):void {
  $B2: {
    %6:ptr<storage, array<u32>, read_write> = access %param, 0u
    %7:u32 = access %param, 1u
    %8:u32 = access %param, 2u
    %9:u32 = access %param, 3u
    %10:u32 = access %param, 4u
    %11:u32 = arrayLength %6
    %12:bool = eq %10, 0u
    %13:u32 = select %10, %11, %12
    %14:bool = eq %9, 0u
    %15:u32 = select %9, %13, %14
    %16:u32 = select 0u, %7, %14
    %17:u32 = sub %15, %16
    %18:u32 = sub %17, %8
    %19:u32 = div %18, 4u
    ret
  }
}
%foo = func():void {
  $B3: {
    %21:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv1, 16u
    %22:buffer_bundle_0 = construct %21, 16u, 0u, 64u, 0u
    %23:void = call %bar, %22
    %24:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv2, 32u
    %25:buffer_bundle_0 = construct %24, 32u, 0u, 0u, 0u
    %26:void = call %bar, %25
    %27:buffer_bundle_0 = construct %gv3, 0u, 0u, 0u, 0u
    %28:void = call %bar, %27
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, FunctionCall_MultiCallSite_SameCaller_WithLets) {
    auto* gv1 = b.Var("gv1", ty.ptr(storage, ty.unsized_buffer()));
    gv1->SetBindingPoint(0, 0);
    mod.root_block->Append(gv1);
    auto* gv2 = b.Var("gv2", ty.ptr(storage, ty.buffer(128)));
    gv2->SetBindingPoint(0, 0);
    mod.root_block->Append(gv2);
    auto* gv3 = b.Var("gv3", ty.ptr(storage, ty.runtime_array(ty.u32())));
    gv3->SetBindingPoint(0, 0);
    mod.root_block->Append(gv3);

    auto* bar = b.Function("bar", ty.void_());
    auto* param = b.FunctionParam("param", ty.ptr(storage, ty.runtime_array(ty.u32())));
    bar->SetParams({param});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, param);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* v1 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, gv1, 16_u, 64_u);
        auto* l1 = b.Let("l1", v1);
        b.Call(ty.void_(), bar, l1);
        auto* l2 = b.Let("l2", gv2);
        auto* v2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferView, Vector{ty.runtime_array(ty.u32())},
                                  l2, 32_u);
        b.Call(ty.void_(), bar, v2);
        b.Call(ty.void_(), bar, gv3);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv1:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, buffer<128>, read_write> = var undef @binding_point(0, 0)
  %gv3:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%param:ptr<storage, array<u32>, read_write>):void {
  $B2: {
    %6:u32 = arrayLength %param
    ret
  }
}
%foo = func():void {
  $B3: {
    %8:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv1, 16u, 64u
    %l1:ptr<storage, array<u32>, read_write> = let %8
    %10:void = call %bar, %l1
    %l2:ptr<storage, buffer<128>, read_write> = let %gv2
    %12:ptr<storage, array<u32>, read_write> = bufferView<array<u32>> %l2, 32u
    %13:void = call %bar, %12
    %14:void = call %bar, %gv3
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv1:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, array<u8, 128>, read_write> = var undef @binding_point(0, 0)
  %gv3:ptr<storage, array<u32>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%param:buffer_bundle_0):void {
  $B2: {
    %6:ptr<storage, array<u32>, read_write> = access %param, 0u
    %7:u32 = access %param, 1u
    %8:u32 = access %param, 2u
    %9:u32 = access %param, 3u
    %10:u32 = access %param, 4u
    %11:u32 = arrayLength %6
    %12:bool = eq %10, 0u
    %13:u32 = select %10, %11, %12
    %14:bool = eq %9, 0u
    %15:u32 = select %9, %13, %14
    %16:u32 = select 0u, %7, %14
    %17:u32 = sub %15, %16
    %18:u32 = sub %17, %8
    %19:u32 = div %18, 4u
    ret
  }
}
%foo = func():void {
  $B3: {
    %21:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv1, 16u
    %22:buffer_bundle_0 = construct %21, 16u, 0u, 64u, 0u
    %23:void = call %bar, %22
    %24:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv2, 32u
    %25:buffer_bundle_0 = construct %24, 32u, 0u, 0u, 0u
    %26:void = call %bar, %25
    %27:buffer_bundle_0 = construct %gv3, 0u, 0u, 0u, 0u
    %28:void = call %bar, %27
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, FunctionCall_MultiParamsModified) {
    auto* gv1 = b.Var("gv1", ty.ptr(storage, ty.unsized_buffer()));
    gv1->SetBindingPoint(0, 0);
    mod.root_block->Append(gv1);
    auto* gv2 = b.Var("gv2", ty.ptr(storage, ty.buffer(128)));
    gv2->SetBindingPoint(0, 0);
    mod.root_block->Append(gv2);

    auto* bar = b.Function("bar", ty.void_());
    auto* p1 = b.FunctionParam("p1", ty.ptr(storage, ty.runtime_array(ty.u32())));
    auto* p2 = b.FunctionParam("p2", ty.ptr(storage, ty.runtime_array(ty.vec2(ty.u32()))));
    bar->SetParams({p1, p2});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, p2);
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, p1);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* v1 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, gv1, 16_u, 64_u);
        auto* l1 = b.Let("l1", v1);
        auto* l2 = b.Let("l2", gv2);
        auto* v2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2(ty.u32()))),
                                  core::BuiltinFn::kBufferView,
                                  Vector{ty.runtime_array(ty.vec2(ty.u32()))}, l2, 32_u);
        b.Call(ty.void_(), bar, l1, v2);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv1:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, buffer<128>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%p1:ptr<storage, array<u32>, read_write>, %p2:ptr<storage, array<vec2<u32>>, read_write>):void {
  $B2: {
    %6:u32 = arrayLength %p2
    %7:u32 = arrayLength %p1
    ret
  }
}
%foo = func():void {
  $B3: {
    %9:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv1, 16u, 64u
    %l1:ptr<storage, array<u32>, read_write> = let %9
    %l2:ptr<storage, buffer<128>, read_write> = let %gv2
    %12:ptr<storage, array<vec2<u32>>, read_write> = bufferView<array<vec2<u32>>> %l2, 32u
    %13:void = call %bar, %l1, %12
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

buffer_bundle_1 = struct @align(4) {
  buffer:ptr<storage, array<vec2<u32>>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv1:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, array<u8, 128>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%p1:buffer_bundle_0, %p2:buffer_bundle_1):void {
  $B2: {
    %6:ptr<storage, array<vec2<u32>>, read_write> = access %p2, 0u
    %7:u32 = access %p2, 1u
    %8:u32 = access %p2, 2u
    %9:u32 = access %p2, 3u
    %10:u32 = access %p2, 4u
    %11:ptr<storage, array<u32>, read_write> = access %p1, 0u
    %12:u32 = access %p1, 1u
    %13:u32 = access %p1, 2u
    %14:u32 = access %p1, 3u
    %15:u32 = access %p1, 4u
    %16:u32 = arrayLength %6
    %17:bool = eq %10, 0u
    %18:u32 = select %10, %16, %17
    %19:bool = eq %9, 0u
    %20:u32 = select %9, %18, %19
    %21:u32 = select 0u, %7, %19
    %22:u32 = sub %20, %21
    %23:u32 = sub %22, %8
    %24:u32 = div %23, 8u
    %25:u32 = arrayLength %11
    %26:bool = eq %15, 0u
    %27:u32 = select %15, %25, %26
    %28:bool = eq %14, 0u
    %29:u32 = select %14, %27, %28
    %30:u32 = select 0u, %12, %28
    %31:u32 = sub %29, %30
    %32:u32 = sub %31, %13
    %33:u32 = div %32, 4u
    ret
  }
}
%foo = func():void {
  $B3: {
    %35:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv1, 16u
    %36:ptr<storage, array<vec2<u32>>, read_write> = msl.pointer_offset<array<vec2<u32>>> %gv2, 32u
    %37:buffer_bundle_0 = construct %35, 16u, 0u, 64u, 0u
    %38:buffer_bundle_1 = construct %36, 32u, 0u, 0u, 0u
    %39:void = call %bar, %37, %38
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, FunctionCall_MultiParamsModified_MultipleCallers) {
    auto* gv1 = b.Var("gv1", ty.ptr(storage, ty.unsized_buffer()));
    gv1->SetBindingPoint(0, 0);
    mod.root_block->Append(gv1);
    auto* gv2 = b.Var("gv2", ty.ptr(storage, ty.buffer(128)));
    gv2->SetBindingPoint(0, 0);
    mod.root_block->Append(gv2);

    auto* bar = b.Function("bar", ty.void_());
    auto* p1 = b.FunctionParam("p1", ty.ptr(storage, ty.runtime_array(ty.u32())));
    auto* p2 = b.FunctionParam("p2", ty.ptr(storage, ty.runtime_array(ty.vec2(ty.u32()))));
    bar->SetParams({p1, p2});
    b.Append(bar->Block(), [&] {
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, p2);
        b.Call(ty.u32(), core::BuiltinFn::kArrayLength, p1);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* v1 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.u32())}, gv1, 16_u, 64_u);
        auto* l1 = b.Let("l1", v1);
        auto* l2 = b.Let("l2", gv2);
        auto* v2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2(ty.u32()))),
                                  core::BuiltinFn::kBufferView,
                                  Vector{ty.runtime_array(ty.vec2(ty.u32()))}, l2, 32_u);
        b.Call(ty.void_(), bar, l1, v2);
        b.Return(foo);
    });

    auto* foobar = b.Function("foobar", ty.void_());
    b.Append(foobar->Block(), [&] {
        auto* v1 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.vec2(ty.u32()))),
                                  core::BuiltinFn::kBufferArrayView,
                                  Vector{ty.runtime_array(ty.vec2(ty.u32()))}, gv1, 16_u, 64_u);
        auto* l1_1 = b.Let("l1_1", v1);
        auto* l2_1 = b.Let("l2_1", gv2);
        auto* v2 = b.CallExplicit(ty.ptr(storage, ty.runtime_array(ty.u32())),
                                  core::BuiltinFn::kBufferView, Vector{ty.runtime_array(ty.u32())},
                                  l2_1, 32_u);
        b.Call(ty.void_(), bar, v2, l1_1);
        b.Return(foobar);
    });

    auto* src = R"(
$B1: {  # root
  %gv1:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, buffer<128>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%p1:ptr<storage, array<u32>, read_write>, %p2:ptr<storage, array<vec2<u32>>, read_write>):void {
  $B2: {
    %6:u32 = arrayLength %p2
    %7:u32 = arrayLength %p1
    ret
  }
}
%foo = func():void {
  $B3: {
    %9:ptr<storage, array<u32>, read_write> = bufferArrayView<array<u32>> %gv1, 16u, 64u
    %l1:ptr<storage, array<u32>, read_write> = let %9
    %l2:ptr<storage, buffer<128>, read_write> = let %gv2
    %12:ptr<storage, array<vec2<u32>>, read_write> = bufferView<array<vec2<u32>>> %l2, 32u
    %13:void = call %bar, %l1, %12
    ret
  }
}
%foobar = func():void {
  $B4: {
    %15:ptr<storage, array<vec2<u32>>, read_write> = bufferArrayView<array<vec2<u32>>> %gv1, 16u, 64u
    %l1_1:ptr<storage, array<vec2<u32>>, read_write> = let %15
    %l2_1:ptr<storage, buffer<128>, read_write> = let %gv2
    %18:ptr<storage, array<u32>, read_write> = bufferView<array<u32>> %l2_1, 32u
    %19:void = call %bar, %18, %l1_1
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
buffer_bundle_0 = struct @align(4) {
  buffer:ptr<storage, array<vec2<u32>>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

buffer_bundle_1 = struct @align(4) {
  buffer:ptr<storage, array<u32>, read_write> @offset(0)
  offset:u32 @offset(0)
  struct_offset:u32 @offset(4)
  size:u32 @offset(8)
  length:u32 @offset(12)
}

$B1: {  # root
  %gv1:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
  %gv2:ptr<storage, array<u8, 128>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%p1:buffer_bundle_1, %p2:buffer_bundle_0):void {
  $B2: {
    %6:ptr<storage, array<u32>, read_write> = access %p1, 0u
    %7:u32 = access %p1, 1u
    %8:u32 = access %p1, 2u
    %9:u32 = access %p1, 3u
    %10:u32 = access %p1, 4u
    %11:ptr<storage, array<vec2<u32>>, read_write> = access %p2, 0u
    %12:u32 = access %p2, 1u
    %13:u32 = access %p2, 2u
    %14:u32 = access %p2, 3u
    %15:u32 = access %p2, 4u
    %16:u32 = arrayLength %11
    %17:bool = eq %15, 0u
    %18:u32 = select %15, %16, %17
    %19:bool = eq %14, 0u
    %20:u32 = select %14, %18, %19
    %21:u32 = select 0u, %12, %19
    %22:u32 = sub %20, %21
    %23:u32 = sub %22, %13
    %24:u32 = div %23, 8u
    %25:u32 = arrayLength %6
    %26:bool = eq %10, 0u
    %27:u32 = select %10, %25, %26
    %28:bool = eq %9, 0u
    %29:u32 = select %9, %27, %28
    %30:u32 = select 0u, %7, %28
    %31:u32 = sub %29, %30
    %32:u32 = sub %31, %8
    %33:u32 = div %32, 4u
    ret
  }
}
%foo = func():void {
  $B3: {
    %35:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv1, 16u
    %36:ptr<storage, array<vec2<u32>>, read_write> = msl.pointer_offset<array<vec2<u32>>> %gv2, 32u
    %37:buffer_bundle_1 = construct %35, 16u, 0u, 64u, 0u
    %38:buffer_bundle_0 = construct %36, 32u, 0u, 0u, 0u
    %39:void = call %bar, %37, %38
    ret
  }
}
%foobar = func():void {
  $B4: {
    %41:ptr<storage, array<vec2<u32>>, read_write> = msl.pointer_offset<array<vec2<u32>>> %gv1, 16u
    %42:ptr<storage, array<u32>, read_write> = msl.pointer_offset<array<u32>> %gv2, 32u
    %43:buffer_bundle_0 = construct %41, 16u, 0u, 64u, 0u
    %44:buffer_bundle_1 = construct %42, 32u, 0u, 0u, 0u
    %45:void = call %bar, %44, %43
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DecomposeBufferTest, BufferViewInFunction_MultiCaller_NoArrayLength) {
    auto* gv1 = b.Var("gv1", ty.ptr(storage, ty.unsized_buffer()));
    gv1->SetBindingPoint(0, 0);
    mod.root_block->Append(gv1);

    auto* bar = b.Function("bar", ty.void_());
    auto* bar_p = b.FunctionParam("bar_p", ty.ptr(storage, ty.unsized_buffer()));
    bar->SetParams({bar_p});
    b.Append(bar->Block(), [&] {
        b.CallExplicit(ty.ptr(storage, ty.u32()), core::BuiltinFn::kBufferView, Vector{ty.u32()},
                       bar_p, 16_u);
        b.Return(bar);
    });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        b.Call(ty.void_(), bar, gv1);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %gv1:ptr<storage, buffer, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%bar_p:ptr<storage, buffer, read_write>):void {
  $B2: {
    %4:ptr<storage, u32, read_write> = bufferView<u32> %bar_p, 16u
    ret
  }
}
%foo = func():void {
  $B3: {
    %6:void = call %bar, %gv1
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %gv1:ptr<storage, array<u8>, read_write> = var undef @binding_point(0, 0)
}

%bar = func(%bar_p:ptr<storage, array<u8>, read_write>):void {
  $B2: {
    %4:ptr<storage, u32, read_write> = msl.pointer_offset<u32> %bar_p, 16u
    ret
  }
}
%foo = func():void {
  $B3: {
    %6:void = call %bar, %gv1
    ret
  }
}
)";

    Run(DecomposeBuffer);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::msl::writer::raise
