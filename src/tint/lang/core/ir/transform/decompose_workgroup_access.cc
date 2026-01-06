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

#include "src/tint/lang/core/ir/transform/decompose_workgroup_access.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::core::ir::transform {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

/// PIMPL state for the transform.
///
/// This transform rewrites memory accesses to workgroup variables that contain f16 types.
/// These variables are rewritten to use array<vec4<f16>> storage (8 bytes per element) to
/// avoid race conditions when multiple threads access different f16 values that would share
/// the same u32 storage.
///
/// Non-f16 workgroup variables stay unchanged. Future patches will extend this transform to
/// decompose non-f16 workgroup storage (i32/u32/f32), treating it as a vec4<u32> array.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    using VarTypePair = std::pair<core::ir::Var*, const core::type::Type*>;

    /// Maps a (var, type) pair to the generated load helper function for structs, arrays, and
    /// matrices.
    Hashmap<VarTypePair, core::ir::Function*, 2> var_and_type_to_load_fn_{};
    /// Maps a (var, type) pair to the generated store helper function for structs, arrays, and
    /// matrices.
    Hashmap<VarTypePair, core::ir::Function*, 2> var_and_type_to_store_fn_{};

    /// Returns true if `type` is or contains a f16 type.
    bool ContainsF16Type(const core::type::Type* type) {
        if (type->Is<core::type::F16>()) {
            return true;
        }

        if (type->IsScalar()) {
            return false;
        }

        if (const auto* str = type->As<core::type::Struct>()) {
            for (auto* member : str->Members()) {
                if (ContainsF16Type(member->Type())) {
                    return true;
                }
            }
            return false;
        }

        return ContainsF16Type(type->DeepestElement());
    }

    /// Process the module. Only handles workgroup address space variables that contain f16 types.
    void Process() {
        Vector<core::ir::Var*, 4> var_worklist;
        for (auto* inst : *ir.root_block) {
            // Allow this to run before or after PromoteInitializers by handling non-var root_block
            // entries
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }

            // DecomposeStorageAccess maybe have converte the var pointers into ByteAddressBuffer
            // objects. Since they've been changed, then they're Storage buffers and we don't care
            // about them here.
            auto* var_ty = var->Result()->Type()->As<core::type::Pointer>();
            if (!var_ty) {
                continue;
            }

            // Only care about workgroup address space variables.
            if (var_ty->AddressSpace() != core::AddressSpace::kWorkgroup) {
                continue;
            }

            // Only process variables that contain f16 types
            // (to avoid race conditions). Non-f16 workgroup variables don't need decomposition.
            if (!ContainsF16Type(var_ty->StoreType())) {
                continue;
            }

            var_worklist.Push(var);
        }

        for (auto* var : var_worklist) {
            auto* result = var->Result();

            auto usage_worklist = result->UsagesSorted();
            auto* var_ty = result->Type()->As<core::type::Pointer>();
            while (!usage_worklist.IsEmpty()) {
                auto usage = usage_worklist.Pop();
                auto* inst = usage.instruction;

                // Load instructions can be destroyed by the replacing access function
                if (!inst->Alive()) {
                    continue;
                }

                OffsetData od{};
                tint::Switch(
                    inst,  //
                    [&](core::ir::LoadVectorElement* l) { LoadVectorElement(l, var, od); },
                    [&](core::ir::Load* l) { Load(l, var, od); },
                    [&](core::ir::StoreVectorElement* s) { StoreVectorElement(s, var, od); },
                    [&](core::ir::Store* s) { Store(s, var, od); },
                    [&](core::ir::Access* a) { Access(a, var, a->Object()->Type(), od); },
                    [&](core::ir::Let* let) {
                        // The `let` is, essentially, an alias for the `var` as it's assigned
                        // directly. Gather all the `let` usages into our worklist, and then replace
                        // the `let` with the `var` itself.
                        for (auto& use : let->Result()->UsagesSorted()) {
                            usage_worklist.Push(use);
                        }
                        let->Result()->ReplaceAllUsesWith(result);
                        let->Destroy();
                    },
                    TINT_ICE_ON_NO_MATCH);
            }

            // Swap the result type of the `var` to the new result type
            // f16 storage mode: array<vec4<f16>> - 8 bytes per element
            auto array_length = (var_ty->StoreType()->Size() + 7) / 8;
            result->SetType(ty.ptr(var_ty->AddressSpace(), ty.array(ty.vec4h(), array_length),
                                   var_ty->Access()));
        }
    }

    // OffsetData represents an unsigned integer expression.
    // The value is the sum of a const part, held in `byte_offset`, and
    // non-const parts in `byte_offset_expr`.
    struct OffsetData {
        uint32_t byte_offset = 0;
        Vector<core::ir::Value*, 4> byte_offset_expr{};
    };

    // Note, must be called inside a builder insert block (Append, InsertBefore, etc)
    core::ir::Value* OffsetToValue(OffsetData offset) {
        core::ir::Value* val = nullptr;

        // If the offset is zero, skip setting val. This way, we won't add `0 +` and create useless
        // addition expressions, but if the offset is zero, and there are no expressions, make sure
        // we return the 0 value.
        if (offset.byte_offset != 0) {
            val = b.Value(u32(offset.byte_offset));
        } else if (offset.byte_offset_expr.IsEmpty()) {
            return b.Value(0_u);
        }

        for (core::ir::Value* expr : offset.byte_offset_expr) {
            if (!val) {
                val = expr;
            } else {
                val = b.Add(val, expr)->Result();
            }
        }

        return val;
    }

    // Converts a byte offset to an index into a vec4<f16> array (8 bytes per element).
    // Used for f16 storage mode.
    // Note, this must be called inside a builder insert block (Append, InsertBefore, etc)
    core::ir::Value* ByteOffsetToVec4hArrayIndex(core::ir::Value* val) {
        if (auto* cnst = val->As<core::ir::Constant>()) {
            auto v = cnst->Value()->ValueAs<uint32_t>();
            return b.Value(u32(v / 8u));
        }
        return b.Divide(val, 8_u)->Result();
    }

    // Calculates the index of the vec4<f16> element containing the byte at (byte_idx % 8).
    // Used for f16 storage mode. Each element is 2 bytes.
    core::ir::Value* ByteOffsetToVec4hElementIndex(core::ir::Value* byte_idx) {
        if (auto* byte_cnst = byte_idx->As<core::ir::Constant>()) {
            return b.Value(u32((byte_cnst->Value()->ValueAs<uint32_t>() % 8u) / 2u));
        }
        // (byte_idx % 8) / 2 = (byte_idx & 7) >> 1
        return b.ShiftRight(b.And(byte_idx, 7_u), 1_u)->Result();
    }

    void Access(core::ir::Access* a,
                core::ir::Var* var,
                const core::type::Type* obj_ty,
                OffsetData offset) {
        // Note, because we recurse through the `access` helper, the object passed in isn't
        // necessarily the originating `var` object, but maybe a partially resolved access chain
        // object.
        if (auto* view = obj_ty->As<core::type::MemoryView>()) {
            obj_ty = view->StoreType();
        }

        auto update_offset = [&](core::ir::Value* idx_value, uint32_t size) {
            tint::Switch(
                idx_value,
                [&](core::ir::Constant* cnst) {
                    uint32_t idx = cnst->Value()->ValueAs<uint32_t>();
                    offset.byte_offset += size * idx;
                },
                [&](core::ir::Value* val) {
                    b.InsertBefore(a, [&] {
                        offset.byte_offset_expr.Push(
                            b.Multiply(u32(size), b.InsertConvertIfNeeded(ty.u32(), val))
                                ->Result());
                    });
                });
        };

        for (auto* idx_value : a->Indices()) {
            tint::Switch(
                obj_ty,
                [&](const core::type::Vector* v) {
                    update_offset(idx_value, v->Type()->Size());
                    obj_ty = v->Type();
                },
                [&](const core::type::Matrix* m) {
                    update_offset(idx_value, m->ColumnStride());
                    obj_ty = m->ColumnType();
                },
                [&](const core::type::Array* ary) {
                    update_offset(idx_value, ary->ImplicitStride());
                    obj_ty = ary->ElemType();
                },
                [&](const core::type::Struct* s) {
                    auto* cnst = idx_value->As<core::ir::Constant>();

                    // A struct index must be a constant
                    TINT_IR_ASSERT(ir, cnst);

                    uint32_t idx = cnst->Value()->ValueAs<uint32_t>();
                    auto* mem = s->Members()[idx];
                    obj_ty = mem->Type();
                    offset.byte_offset += mem->Offset();
                },
                TINT_ICE_ON_NO_MATCH);
        }

        auto usages = a->Result()->UsagesSorted();
        while (!usages.IsEmpty()) {
            auto usage = usages.Pop();
            tint::Switch(
                usage.instruction,
                [&](core::ir::Let* let) {
                    // The `let` is essentially an alias to the `access`. So, add the `let`
                    // usages into the usage worklist, and replace the let with the access chain
                    // directly.
                    for (auto& u : let->Result()->UsagesSorted()) {
                        usages.Push(u);
                    }
                    let->Result()->ReplaceAllUsesWith(a->Result());
                    let->Destroy();
                },
                [&](core::ir::Access* sub_access) {
                    // Treat an access chain of the access chain as a continuation of the outer
                    // chain. Pass through the object we stopped at and the current byte_offset
                    // and then restart the access chain replacement for the new access chain.
                    Access(sub_access, var, obj_ty, offset);
                },
                [&](core::ir::Load* ld) {
                    a->Result()->RemoveUsage(usage);
                    Load(ld, var, offset);
                },
                [&](core::ir::LoadVectorElement* lve) {
                    a->Result()->RemoveUsage(usage);
                    LoadVectorElement(lve, var, offset);
                },
                [&](core::ir::Store* s) {
                    a->Result()->RemoveUsage(usage);
                    Store(s, var, offset);
                },
                [&](core::ir::StoreVectorElement* sve) {
                    a->Result()->RemoveUsage(usage);
                    StoreVectorElement(sve, var, offset);
                },
                TINT_ICE_ON_NO_MATCH);
        }
        a->Destroy();
    }

    void Load(core::ir::Load* ld, core::ir::Var* var, OffsetData offset) {
        b.InsertBefore(ld, [&] {
            auto* byte_idx = OffsetToValue(offset);
            auto* result = MakeLoad(ld, var, ld->Result()->Type(), byte_idx);
            ld->Result()->ReplaceAllUsesWith(result->Result());
        });
        ld->Destroy();
    }

    void LoadVectorElement(core::ir::LoadVectorElement* lve,
                           core::ir::Var* var,
                           OffsetData offset) {
        b.InsertBefore(lve, [&] {
            // Add the byte count from the start of the vector to the requested element to the
            // current offset calculation
            auto elem_byte_size = lve->Result()->Type()->DeepestElement()->Size();
            if (auto* cnst = lve->Index()->As<core::ir::Constant>()) {
                offset.byte_offset += (cnst->Value()->ValueAs<uint32_t>() * elem_byte_size);
            } else {
                offset.byte_offset_expr.Push(
                    b.Multiply(b.InsertConvertIfNeeded(ty.u32(), lve->Index()), u32(elem_byte_size))
                        ->Result());
            }

            auto* byte_idx = OffsetToValue(offset);
            auto* result = MakeLoad(lve, var, lve->Result()->Type(), byte_idx);
            lve->Result()->ReplaceAllUsesWith(result->Result());
        });
        lve->Destroy();
    }

    void Store(core::ir::Store* s, core::ir::Var* var, OffsetData offset) {
        b.InsertBefore(s, [&] {
            auto* byte_idx = OffsetToValue(offset);
            MakeStore(s, var, s->To()->Type()->UnwrapPtr(), byte_idx, s->From());
        });
        s->Destroy();
    }

    void StoreVectorElement(core::ir::StoreVectorElement* sve,
                            core::ir::Var* var,
                            OffsetData offset) {
        b.InsertBefore(sve, [&] {
            // Add the byte count from the start of the vector to the requested element to the
            // current offset calculation
            auto elem_byte_size = sve->To()->Type()->UnwrapPtr()->DeepestElement()->Size();
            if (auto* cnst = sve->Index()->As<core::ir::Constant>()) {
                offset.byte_offset += (cnst->Value()->ValueAs<uint32_t>() * elem_byte_size);
            } else {
                offset.byte_offset_expr.Push(
                    b.Multiply(b.InsertConvertIfNeeded(ty.u32(), sve->Index()), u32(elem_byte_size))
                        ->Result());
            }

            auto* byte_idx = OffsetToValue(offset);
            MakeStore(sve, var, sve->To()->Type()->UnwrapPtr()->DeepestElement(), byte_idx,
                      sve->Value());
        });
        sve->Destroy();
    }

    // Creates the appropriate load instructions for the given result type.
    // Only f16 storage mode (workgroup with f16) is supported.
    core::ir::Instruction* MakeLoad(core::ir::Instruction* inst,
                                    core::ir::Var* var,
                                    const core::type::Type* result_ty,
                                    core::ir::Value* byte_idx) {
        // f16 storage mode: array<vec4<f16>>
        if (result_ty->IsFloatScalar() || result_ty->IsIntegerScalar()) {
            return MakeScalarLoadFromVec4h(var, result_ty, byte_idx);
        }
        return tint::Switch(
            result_ty,
            [&](const core::type::Struct* s) {
                auto* fn = GetLoadFunctionFor(inst, var, s);
                return b.Call(fn, byte_idx);
            },
            [&](const core::type::Matrix* m) {
                auto* fn = GetLoadFunctionFor(inst, var, m);
                return b.Call(fn, byte_idx);
            },
            [&](const core::type::Array* a) {
                auto* fn = GetLoadFunctionFor(inst, var, a);
                return b.Call(fn, byte_idx);
            },
            [&](const core::type::Vector* v) { return MakeVectorLoadFromVec4h(var, v, byte_idx); },
            TINT_ICE_ON_NO_MATCH);
    }

    // Loads a scalar value from f16 storage mode (array<vec4<f16>>).
    // For f16 types: direct load from vec4<f16>
    // For 32-bit types: load 2 adjacent f16 values and bitcast to the target type
    core::ir::Instruction* MakeScalarLoadFromVec4h(core::ir::Var* var,
                                                   const core::type::Type* result_ty,
                                                   core::ir::Value* byte_idx) {
        auto* array_idx = ByteOffsetToVec4hArrayIndex(byte_idx);
        auto* access = b.Access(ty.ptr(workgroup, ty.vec4h()), var, array_idx);

        auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);

        if (result_ty->Is<core::type::F16>()) {
            // Direct load for f16
            return b.LoadVectorElement(access, vec_idx);
        }

        // For 32-bit types, load 2 adjacent f16 values and bitcast
        // vec_idx points to the first f16, vec_idx+1 is the second
        core::ir::Value* val_x = nullptr;
        core::ir::Value* val_y = nullptr;

        if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
            auto idx = cnst->Value()->ValueAs<uint32_t>();
            val_x = b.LoadVectorElement(access, u32(idx))->Result();
            val_y = b.LoadVectorElement(access, u32(idx + 1))->Result();
        } else {
            val_x = b.LoadVectorElement(access, vec_idx)->Result();
            auto* next_idx = b.Add(vec_idx, 1_u)->Result();
            val_y = b.LoadVectorElement(access, next_idx)->Result();
        }

        auto* vec2h = b.Construct<vec2<f16>>(val_x, val_y);
        return b.Bitcast(result_ty, vec2h);
    }

    // Loads a vector from f16 storage mode (array<vec4<f16>>).
    // For vec<f16> types: direct load/swizzle from vec4<f16>
    // For vec<32-bit> types: load multiple f16 values and bitcast to the target type
    core::ir::Instruction* MakeVectorLoadFromVec4h(core::ir::Var* var,
                                                   const core::type::Vector* result_ty,
                                                   core::ir::Value* byte_idx) {
        auto* array_idx = ByteOffsetToVec4hArrayIndex(byte_idx);
        auto* access = b.Access(ty.ptr(workgroup, ty.vec4h()), var, array_idx);
        auto width = result_ty->Width();

        // --- vec<f16> types: direct load/swizzle ---
        if (result_ty->DeepestElement()->Is<core::type::F16>()) {
            auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);
            switch (width) {
                case 4:
                    return b.Load(access);
                case 3:
                    return b.Swizzle(result_ty, b.Load(access), {0, 1, 2});
                case 2: {
                    if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                        auto swizzle = cnst->Value()->ValueAs<uint32_t>() >= 2
                                           ? Vector<uint32_t, 2>{2, 3}
                                           : Vector<uint32_t, 2>{0, 1};
                        return b.Swizzle(result_ty, b.Load(access), swizzle);
                    }
                    auto* loaded = b.Load(access);
                    auto* cond = b.GreaterThanEqual(vec_idx, 2_u);
                    return b.Call(
                        result_ty, core::BuiltinFn::kSelect,
                        Vector{b.Swizzle(result_ty, loaded, {0, 1})->Result(),
                               b.Swizzle(result_ty, loaded, {2, 3})->Result(), cond->Result()});
                }
                default:
                    TINT_IR_UNREACHABLE(ir);
            }
        }

        // --- vec<32-bit> types: load f16 pairs and bitcast ---
        // Helper: extract pair of f16 values from a vec4<f16> and bitcast to 32-bit element type
        auto bitcast_pair = [&](core::ir::Value* vec, uint32_t idx0, uint32_t idx1) {
            auto* h0 = b.Access<f16>(vec, u32(idx0))->Result();
            auto* h1 = b.Access<f16>(vec, u32(idx1))->Result();
            return b.Bitcast(result_ty->Type(), b.Construct<vec2<f16>>(h0, h1))->Result();
        };

        auto* v0 = b.Load(access)->Result();

        if (width == 4 || width == 3) {
            // vec3/vec4<32-bit> need elements from 2 vec4<f16> array slots
            core::ir::Value* next_array_idx = nullptr;
            if (auto* cnst = array_idx->As<core::ir::Constant>()) {
                next_array_idx = b.Value(u32(cnst->Value()->ValueAs<uint32_t>() + 1));
            } else {
                next_array_idx = b.Add(array_idx, 1_u)->Result();
            }
            auto* v1 =
                b.Load(b.Access(ty.ptr(workgroup, ty.vec4h()), var, next_array_idx))->Result();

            if (width == 4) {
                return b.Construct(result_ty,
                                   Vector{bitcast_pair(v0, 0, 1), bitcast_pair(v0, 2, 3),
                                          bitcast_pair(v1, 0, 1), bitcast_pair(v1, 2, 3)});
            }
            // width == 3
            return b.Construct(result_ty, Vector{bitcast_pair(v0, 0, 1), bitcast_pair(v0, 2, 3),
                                                 bitcast_pair(v1, 0, 1)});
        }

        // width == 2: vec2<32-bit> = 4 f16 values, fits in one vec4<f16>
        auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);
        if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
            auto base = cnst->Value()->ValueAs<uint32_t>();
            return b.Construct(result_ty, Vector{bitcast_pair(v0, base, base + 1),
                                                 bitcast_pair(v0, base + 2, base + 3)});
        }
        // Dynamic index case
        Vector<core::ir::Value*, 2> elems;
        for (uint32_t i = 0; i < 2; ++i) {
            auto* idx0 = b.Add(vec_idx, u32(i * 2))->Result();
            auto* idx1 = b.Add(vec_idx, u32(i * 2 + 1))->Result();
            auto* h0 = b.Access<f16>(v0, idx0)->Result();
            auto* h1 = b.Access<f16>(v0, idx1)->Result();
            elems.Push(b.Bitcast(result_ty->Type(), b.Construct<vec2<f16>>(h0, h1))->Result());
        }
        return b.Construct(result_ty, elems);
    }

    // Creates a load function for the given `var` and `matrix` combination. Essentially creates
    // a function similar to:
    //
    // fn custom_load_M(offset: u32) {
    //   const uint scalar_offset = ((offset + 0u)) / 4;
    //   const uint scalar_offset_1 = ((offset + (1 * ColumnStride))) / 4;
    //   const uint scalar_offset_2 = ((offset + (2 * ColumnStride))) / 4;
    //   const uint scalar_offset_3 = ((offset + (3 * ColumnStride)))) / 4;
    //   return float4x4(
    //       asfloat(v[scalar_offset / 4]),
    //       asfloat(v[scalar_offset_1 / 4]),
    //       asfloat(v[scalar_offset_2 / 4]),
    //      asfloat(v[scalar_offset_3 / 4])
    //   );
    // }
    core::ir::Function* GetLoadFunctionFor(core::ir::Instruction* inst,
                                           core::ir::Var* var,
                                           const core::type::Matrix* mat) {
        return var_and_type_to_load_fn_.GetOrAdd(VarTypePair{var, mat}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* fn = b.Function(mat);
            fn->SetParams({start_byte_offset});

            b.Append(fn->Block(), [&] {
                Vector<core::ir::Value*, 4> values;
                for (size_t i = 0; i < mat->Columns(); ++i) {
                    uint32_t stride = static_cast<uint32_t>(i * mat->ColumnStride());

                    OffsetData od{stride, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(od);
                    values.Push(MakeLoad(inst, var, mat->ColumnType(), byte_idx)->Result());
                }
                b.Return(fn, b.Construct(mat, values));
            });

            return fn;
        });
    }

    // Creates a load function for the given `var` and `array` combination. Essentially creates
    // a function similar to:
    //
    // fn custom_load_A(offset: u32) {
    //   A a = A();
    //   u32 i = 0;
    //   loop {
    //     if (i >= A length) {
    //       break;
    //     }
    //     offset = (offset + (i * A->ImplicitStride())) / 16
    //     a[i] = cast(v[offset].xyz)
    //     i = i + 1;
    //   }
    //   return a;
    // }
    core::ir::Function* GetLoadFunctionFor(core::ir::Instruction* inst,
                                           core::ir::Var* var,
                                           const core::type::Array* arr) {
        return var_and_type_to_load_fn_.GetOrAdd(VarTypePair{var, arr}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* fn = b.Function(arr);
            fn->SetParams({start_byte_offset});

            b.Append(fn->Block(), [&] {
                auto* result_arr = b.Var<function>("a", b.Zero(arr));

                auto* count = arr->Count()->As<core::type::ConstantArrayCount>();
                TINT_IR_ASSERT(ir, count);

                b.LoopRange(0_u, u32(count->value), 1_u, [&](core::ir::Value* idx) {
                    auto* stride = b.Multiply(idx, u32(arr->ImplicitStride()))->Result();
                    OffsetData od{0, {start_byte_offset, stride}};
                    auto* byte_idx = OffsetToValue(od);
                    auto* access = b.Access(ty.ptr<function>(arr->ElemType()), result_arr, idx);
                    b.Store(access, MakeLoad(inst, var, arr->ElemType(), byte_idx));
                });

                b.Return(fn, b.Load(result_arr));
            });

            return fn;
        });
    }

    // Creates a load function for the given `var` and `struct` combination. Essentially creates
    // a function similar to:
    //
    // fn custom_load_S(start_offset: u32) {
    //   let a = load object at (start_offset + member_offset)
    //   let b = load object at (start_offset + member 1 offset);
    //   ...
    //   return S(a, b, ..., z);
    // }
    core::ir::Function* GetLoadFunctionFor(core::ir::Instruction* inst,
                                           core::ir::Var* var,
                                           const core::type::Struct* s) {
        return var_and_type_to_load_fn_.GetOrAdd(VarTypePair{var, s}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* fn = b.Function(s);
            fn->SetParams({start_byte_offset});

            b.Append(fn->Block(), [&] {
                Vector<core::ir::Value*, 4> values;
                for (const auto* mem : s->Members()) {
                    uint32_t stride = static_cast<uint32_t>(mem->Offset());

                    OffsetData od{stride, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(od);
                    values.Push(MakeLoad(inst, var, mem->Type(), byte_idx)->Result());
                }

                b.Return(fn, b.Construct(s, values));
            });

            return fn;
        });
    }

    // --- Store Helpers ---
    // Note: Only workgroup address space with f16 types needs store operations.

    void MakeStore(core::ir::Instruction* inst,
                   core::ir::Var* var,
                   const core::type::Type* type,
                   core::ir::Value* byte_idx,
                   core::ir::Value* value) {
        if (type->IsFloatScalar() || type->IsIntegerScalar()) {
            MakeScalarStoreToVec4h(var, type, byte_idx, value);
            return;
        }

        tint::Switch(
            type,
            [&](const core::type::Struct* s) {
                auto* fn = GetStoreFunctionFor(inst, var, s);
                b.Call(fn, byte_idx, value);
            },
            [&](const core::type::Matrix* m) {
                auto* fn = GetStoreFunctionFor(inst, var, m);
                b.Call(fn, byte_idx, value);
            },
            [&](const core::type::Array* a) {
                auto* fn = GetStoreFunctionFor(inst, var, a);
                b.Call(fn, byte_idx, value);
            },
            [&](const core::type::Vector* v) { MakeVectorStoreToVec4h(var, v, byte_idx, value); },
            TINT_ICE_ON_NO_MATCH);
    }

    // Stores a scalar value to f16 storage mode (array<vec4<f16>>).
    // For f16 types: direct store to vec4<f16>
    // For 32-bit types: bitcast to vec2<f16> and store 2 adjacent f16 values
    void MakeScalarStoreToVec4h(core::ir::Var* var,
                                const core::type::Type* type,
                                core::ir::Value* byte_idx,
                                core::ir::Value* value) {
        auto* array_idx = ByteOffsetToVec4hArrayIndex(byte_idx);
        auto* access = b.Access(ty.ptr(workgroup, ty.vec4h()), var, array_idx);
        auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);

        if (type->Is<core::type::F16>()) {
            // Direct store for f16
            b.StoreVectorElement(access, vec_idx, value);
            return;
        }

        // For 32-bit types, bitcast to vec2<f16> and store 2 adjacent f16 values
        auto* vec2h = b.Bitcast<vec2<f16>>(value);
        auto* h0 = b.Access<f16>(vec2h, 0_u)->Result();
        auto* h1 = b.Access<f16>(vec2h, 1_u)->Result();

        if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
            auto idx = cnst->Value()->ValueAs<uint32_t>();
            b.StoreVectorElement(access, u32(idx), h0);
            b.StoreVectorElement(access, u32(idx + 1), h1);
        } else {
            b.StoreVectorElement(access, vec_idx, h0);
            auto* next_idx = b.Add(vec_idx, 1_u)->Result();
            b.StoreVectorElement(access, next_idx, h1);
        }
    }

    // Stores a vector value to f16 storage mode (array<vec4<f16>>).
    // For vec<f16> types: direct store to vec4<f16>
    // For vec<32-bit> types: bitcast each element to vec2<f16> and store
    void MakeVectorStoreToVec4h(core::ir::Var* var,
                                const core::type::Vector* vec_ty,
                                core::ir::Value* byte_idx,
                                core::ir::Value* value) {
        auto* array_idx = ByteOffsetToVec4hArrayIndex(byte_idx);
        auto* access = b.Access(ty.ptr(workgroup, ty.vec4h()), var, array_idx);
        auto width = vec_ty->Width();

        // --- vec<f16> types: direct store ---
        if (vec_ty->DeepestElement()->Is<core::type::F16>()) {
            auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);
            switch (width) {
                case 4:
                    b.Store(access, value);
                    return;
                case 3: {
                    // vec3<f16> - store as vec4<f16> with padding
                    auto* x = b.Access<f16>(value, 0_u)->Result();
                    auto* y = b.Access<f16>(value, 1_u)->Result();
                    auto* z = b.Access<f16>(value, 2_u)->Result();
                    b.Store(access, b.Construct<vec4<f16>>(x, y, z, b.Value(f16(0))));
                    return;
                }
                case 2: {
                    // vec2<f16> - store 2 elements
                    auto* x = b.Access<f16>(value, 0_u)->Result();
                    auto* y = b.Access<f16>(value, 1_u)->Result();
                    if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                        auto idx = cnst->Value()->ValueAs<uint32_t>();
                        b.StoreVectorElement(access, u32(idx), x);
                        b.StoreVectorElement(access, u32(idx + 1), y);
                    } else {
                        b.StoreVectorElement(access, vec_idx, x);
                        b.StoreVectorElement(access, b.Add(vec_idx, 1_u)->Result(), y);
                    }
                    return;
                }
                default:
                    TINT_IR_UNREACHABLE(ir);
            }
        }

        // --- vec<32-bit> types: bitcast to f16 pairs and store ---
        // Helper: extract element, bitcast to vec2<f16>, return the two f16 values
        auto extract_h_pair =
            [&](uint32_t elem_idx) -> std::pair<core::ir::Value*, core::ir::Value*> {
            auto* elem = b.Access(vec_ty->Type(), value, u32(elem_idx))->Result();
            auto* vec2h = b.Bitcast<vec2<f16>>(elem);
            return {b.Access<f16>(vec2h, 0_u)->Result(), b.Access<f16>(vec2h, 1_u)->Result()};
        };

        // Helper: create next array access
        auto next_access = [&]() {
            core::ir::Value* next_idx = nullptr;
            if (auto* cnst = array_idx->As<core::ir::Constant>()) {
                next_idx = b.Value(u32(cnst->Value()->ValueAs<uint32_t>() + 1));
            } else {
                next_idx = b.Add(array_idx, 1_u)->Result();
            }
            return b.Access(ty.ptr(workgroup, ty.vec4h()), var, next_idx);
        };

        if (width == 4 || width == 3) {
            // Extract all elements to f16 values
            Vector<core::ir::Value*, 8> h;
            for (uint32_t i = 0; i < width; ++i) {
                auto [h0, h1] = extract_h_pair(i);
                h.Push(h0);
                h.Push(h1);
            }

            // Store first vec4<f16>
            b.Store(access, b.Construct<vec4<f16>>(h[0], h[1], h[2], h[3]));

            // Store second vec4<f16> (with padding for vec3)
            auto* access2 = next_access();
            if (width == 4) {
                b.Store(access2, b.Construct<vec4<f16>>(h[4], h[5], h[6], h[7]));
            } else {
                b.Store(access2,
                        b.Construct<vec4<f16>>(h[4], h[5], b.Value(f16(0)), b.Value(f16(0))));
            }
            return;
        }

        // width == 2: vec2<32-bit> = 4 f16 values, fits in one vec4<f16>
        auto [h0, h1] = extract_h_pair(0);
        auto [h2, h3] = extract_h_pair(1);

        auto* vec_idx = ByteOffsetToVec4hElementIndex(byte_idx);
        if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
            auto base = cnst->Value()->ValueAs<uint32_t>();
            b.StoreVectorElement(access, u32(base), h0);
            b.StoreVectorElement(access, u32(base + 1), h1);
            b.StoreVectorElement(access, u32(base + 2), h2);
            b.StoreVectorElement(access, u32(base + 3), h3);
        } else {
            Vector<core::ir::Value*, 4> h_values{h0, h1, h2, h3};
            for (uint32_t i = 0; i < 4; ++i) {
                b.StoreVectorElement(access, b.Add(vec_idx, u32(i))->Result(), h_values[i]);
            }
        }
    }

    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Struct* s) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, s}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* val = b.FunctionParam("val", s);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, val});
            b.Append(fn->Block(), [&] {
                for (const auto* mem : s->Members()) {
                    uint32_t stride = static_cast<uint32_t>(mem->Offset());
                    OffsetData od{stride, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(od);
                    auto* mem_val = b.Access(mem->Type(), val, u32(mem->Index()))->Result();
                    MakeStore(inst, var, mem->Type(), byte_idx, mem_val);
                }
                b.Return(fn);
            });
            return fn;
        });
    }

    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Matrix* m) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, m}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* val = b.FunctionParam("val", m);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, val});
            b.Append(fn->Block(), [&] {
                for (size_t i = 0; i < m->Columns(); ++i) {
                    uint32_t stride = static_cast<uint32_t>(i * m->ColumnStride());
                    OffsetData od{stride, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(od);
                    auto* col_val = b.Access(m->ColumnType(), val, u32(i))->Result();
                    MakeStore(inst, var, m->ColumnType(), byte_idx, col_val);
                }
                b.Return(fn);
            });
            return fn;
        });
    }

    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Array* arr) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, arr}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* val = b.FunctionParam("val", arr);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, val});
            b.Append(fn->Block(), [&] {
                auto* count = arr->Count()->As<core::type::ConstantArrayCount>();
                TINT_IR_ASSERT(ir, count);
                b.LoopRange(0_u, u32(count->value), 1_u, [&](core::ir::Value* idx) {
                    auto* stride = b.Multiply(idx, u32(arr->ImplicitStride()))->Result();
                    OffsetData od{0, {start_byte_offset, stride}};
                    auto* byte_idx = OffsetToValue(od);
                    auto* elem_val = b.Access(arr->ElemType(), val, idx)->Result();
                    MakeStore(inst, var, arr->ElemType(), byte_idx, elem_val);
                });
                b.Return(fn);
            });
            return fn;
        });
    }
};

}  // namespace

Result<SuccessType> DecomposeWorkgroupAccess(core::ir::Module& ir) {
    auto result =
        ValidateAndDumpIfNeeded(ir, "core.DecomposeWorkgroupAccess",
                                core::ir::Capabilities{
                                    core::ir::Capability::kAllow8BitIntegers,
                                    core::ir::Capability::kAllowClipDistancesOnF32ScalarAndVector,
                                    core::ir::Capability::kAllowDuplicateBindings,
                                    core::ir::Capability::kAllowNonCoreTypes,
                                });
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
