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

#include "src/tint/lang/core/ir/transform/decompose_memory_access.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::core::ir::transform {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

/// PIMPL state for the transform.
///
/// Each uniform buffer variable is rewritten so its store type is an array of vec4u
/// elements.  Accesses to original store type contents are rewritten to pick out the
/// right set of vec4u elements and then use bitcasts as needed to construct the originally
/// loaded value.
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

    /// Process the module.
    void Process(core::AddressSpace address_space) {
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

            // Only care about the requested address space variables.
            if (var_ty->AddressSpace() != address_space) {
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
                    [&](core::ir::LoadVectorElement* l) { ProcessLoadVectorElement(l, var, od); },
                    [&](core::ir::Load* l) { ProcessLoad(l, var, od); },
                    [&](core::ir::StoreVectorElement* s) { ProcessStoreVectorElement(s, var, od); },
                    [&](core::ir::Store* s) { ProcessStore(s, var, od); },
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
            auto array_length = (var_ty->StoreType()->Size() + 15) / 16;
            result->SetType(ty.ptr(var_ty->AddressSpace(), ty.array(ty.vec4u(), array_length),
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

    // Converts a byte offset to an index into a vec4u array.
    // After the transform runs the store type of the uniform buffer variable is an
    // array of vec4u.
    // Note, this must be called inside a builder insert block (Append, InsertBefore, etc)
    core::ir::Value* OffsetValueToArrayIndex(core::ir::Value* val) {
        if (auto* cnst = val->As<core::ir::Constant>()) {
            auto v = cnst->Value()->ValueAs<uint32_t>();
            return b.Value(u32(v / 16u));
        }
        return b.Divide(val, 16_u)->Result();
    }

    // Calculates the index of the vec4u element containing the byte at (byte_idx % 16).
    // Assumes the upper bits of byte_idx have already been used to access the correct
    // vec4u array element in the underlying variable.
    core::ir::Value* CalculateVectorOffset(core::ir::Value* byte_idx) {
        if (auto* byte_cnst = byte_idx->As<core::ir::Constant>()) {
            return b.Value(u32((byte_cnst->Value()->ValueAs<uint32_t>() % 16u) / 4u));
        }
        // Note: Using bitwise-and and shift instead of modulo and divide here was necessary to
        // avoid an FXC miscompile. See https://crbug.com/454366353.
        return b.ShiftRight(b.And(byte_idx, 15_u), 2_u)->Result();
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
                    ProcessLoad(ld, var, offset);
                },
                [&](core::ir::LoadVectorElement* lve) {
                    a->Result()->RemoveUsage(usage);
                    ProcessLoadVectorElement(lve, var, offset);
                },
                [&](core::ir::Store* s) {
                    a->Result()->RemoveUsage(usage);
                    ProcessStore(s, var, offset);
                },
                [&](core::ir::StoreVectorElement* sve) {
                    a->Result()->RemoveUsage(usage);
                    ProcessStoreVectorElement(sve, var, offset);
                },
                TINT_ICE_ON_NO_MATCH);
        }
        a->Destroy();
    }

    void ProcessLoad(core::ir::Load* ld, core::ir::Var* var, OffsetData offset) {
        b.InsertBefore(ld, [&] {
            auto* byte_idx = OffsetToValue(offset);
            auto* result = MakeLoad(ld, var, ld->Result()->Type(), byte_idx);
            ld->Result()->ReplaceAllUsesWith(result->Result());
        });
        ld->Destroy();
    }

    void ProcessLoadVectorElement(core::ir::LoadVectorElement* lve,
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

    void ProcessStore(core::ir::Store* s, core::ir::Var* var, OffsetData offset) {
        b.InsertBefore(s, [&] {
            auto* byte_idx = OffsetToValue(offset);
            MakeStore(s, var, s->To()->Type()->UnwrapPtr(), byte_idx, s->From());
        });
        s->Destroy();
    }

    void ProcessStoreVectorElement(core::ir::StoreVectorElement* sve,
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
    core::ir::Instruction* MakeLoad(core::ir::Instruction* inst,
                                    core::ir::Var* var,
                                    const core::type::Type* result_ty,
                                    core::ir::Value* byte_idx) {
        if (result_ty->IsFloatScalar() || result_ty->IsIntegerScalar()) {
            return MakeScalarLoad(var, result_ty, byte_idx);
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
            [&](const core::type::Vector* v) { return MakeVectorLoad(var, v, byte_idx); },
            TINT_ICE_ON_NO_MATCH);
    }

    core::ir::Instruction* MakeScalarLoad(core::ir::Var* var,
                                          const core::type::Type* result_ty,
                                          core::ir::Value* byte_idx) {
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto as = var->Result()->Type()->As<core::type::Pointer>()->AddressSpace();
        auto* access = b.Access(ty.ptr(as, ty.vec4u()), var, array_idx);

        auto* vec_idx = CalculateVectorOffset(byte_idx);
        core::ir::Instruction* load = b.LoadVectorElement(access, vec_idx);
        if (result_ty->Is<core::type::F16>()) {
            return ExtractScalarF16(load, byte_idx);
        }
        return b.Bitcast(result_ty, load);
    }

    // Calculates the element index (0 or 1) within a vec2<f16> based on byte offset.
    // If byte_idx % 4 == 0, returns 0 (first element), otherwise returns 1 (second element).
    core::ir::Value* CalculateF16ElementIndex(core::ir::Value* byte_idx) {
        if (auto* cnst = byte_idx->As<core::ir::Constant>()) {
            if (cnst->Value()->ValueAs<uint32_t>() % 4 == 0) {
                return b.Constant(0_u);
            } else {
                return b.Constant(1_u);
            }
        }
        auto* false_ = b.Value(1_u);
        auto* true_ = b.Value(0_u);
        auto* cond = b.Equal(b.Modulo(byte_idx, 4_u), 0_u);

        Vector<core::ir::Value*, 3> args{false_, true_, cond->Result()};
        return b.Call(ty.u32(), core::BuiltinFn::kSelect, args)->Result();
    }

    // Extracts a scalar f16 value from a u32 value at the position determined by byte_idx.
    // The u32 is bitcast to vec2<f16> and the appropriate element is extracted.
    core::ir::Access* ExtractScalarF16(core::ir::Instruction* load, core::ir::Value* byte_idx) {
        auto* element_index = CalculateF16ElementIndex(byte_idx);
        auto* bitcast = b.Bitcast<vec2<f16>>(load);
        return b.Access<f16>(bitcast, element_index);
    }

    // Inserts a scalar f16 value into a u32 value at the position determined by byte_idx.
    // Performs a read-modify-write: loads the existing u32, bitcasts to vec2<f16>,
    // replaces the appropriate element, and returns the new u32 value.
    core::ir::Instruction* InsertScalarF16(core::ir::Access* access,
                                           core::ir::Value* vec_idx,
                                           core::ir::Value* byte_idx,
                                           core::ir::Value* value) {
        auto* element_index = CalculateF16ElementIndex(byte_idx);

        // Load the existing u32 value and bitcast to vec2<f16>
        auto* existing_u32 = b.LoadVectorElement(access, vec_idx);
        auto* existing_vec2h = b.Bitcast<vec2<f16>>(existing_u32);

        // Extract existing elements
        auto* x_elem = b.Access<f16>(existing_vec2h, 0_u)->Result();
        auto* y_elem = b.Access<f16>(existing_vec2h, 1_u)->Result();

        // If element_index == 0, new_x = value, new_y = existing.y
        // If element_index == 1, new_x = existing.x, new_y = value
        auto* is_first_elem = b.Equal(element_index, 0_u);
        auto* new_x = b.Call(ty.f16(), core::BuiltinFn::kSelect,
                             Vector{x_elem, value, is_first_elem->Result()})
                          ->Result();
        auto* new_y = b.Call(ty.f16(), core::BuiltinFn::kSelect,
                             Vector{value, y_elem, is_first_elem->Result()})
                          ->Result();

        // Construct the new vec2<f16> and bitcast back to u32
        auto* new_vec2h = b.Construct<vec2<f16>>(new_x, new_y);
        return b.Bitcast<u32>(new_vec2h);
    }

    // When loading a vector we have to take the alignment into account to determine which part of
    // the `uint4` to load. A `vec`  of `u32`, `f32` or `i32` has an alignment requirement of
    // a multiple of 8-bytes (`f16` is 4-bytes). So, this means we'll have memory like:
    //
    // Byte: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
    // Value:|      v1       |        v2     |        v3       |          v4       |
    // Scalar Index:   0                 1               2                   3
    //
    // Start with a byte address which is `offset + (column * columnStride)`, the array index is
    // `byte_address / 16`. This gives us the `uint4` which contains our values. We can then
    // calculate the vector offset as `(byte_address % 16) / 4`.
    //
    // * A 4-element row we access all 4-elements at the `array_idx`
    // * A 3-element row we access `array_idx` at `.xyz` as it must be padded to a vec4.
    // * A 2-element row, we have to decide if we want the `xy` or `zw` element. We have a minimum
    //   alignment of 8-bytes as per the WGSL spec. So if the `vector_idx != 2` is `0` then we
    //   access the `.xy` component, otherwise it is in the `.zw` component.
    core::ir::Instruction* MakeVectorLoad(core::ir::Var* var,
                                          const core::type::Vector* result_ty,
                                          core::ir::Value* byte_idx) {
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto as = var->Result()->Type()->As<core::type::Pointer>()->AddressSpace();
        auto* access = b.Access(ty.ptr(as, ty.vec4u()), var, array_idx);

        if (result_ty->DeepestElement()->Is<core::type::F16>()) {
            return MakeVectorLoadF16(access, result_ty, byte_idx);
        }

        TINT_IR_ASSERT(ir, result_ty->DeepestElement()->Size() == 4);
        core::ir::Instruction* load = nullptr;
        if (result_ty->Width() == 4) {
            load = b.Load(access);
        } else if (result_ty->Width() == 3) {
            load = b.Swizzle(ty.vec3u(), b.Load(access), {0, 1, 2});
        } else if (result_ty->Width() == 2) {
            auto* vec_idx = CalculateVectorOffset(byte_idx);
            if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                if (cnst->Value()->ValueAs<uint32_t>() == 2u) {
                    load = b.Swizzle(ty.vec2u(), b.Load(access), {2, 3});
                } else {
                    load = b.Swizzle(ty.vec2u(), b.Load(access), {0, 1});
                }
            } else {
                auto* ubo = b.Load(access);
                // if vec_idx == 2 -> zw
                auto* sw_lhs = b.Swizzle(ty.vec2u(), ubo, {2, 3});
                // else -> xy
                auto* sw_rhs = b.Swizzle(ty.vec2u(), ubo, {0, 1});
                auto* cond = b.Equal(vec_idx, 2_u);

                Vector<core::ir::Value*, 3> args{sw_rhs->Result(), sw_lhs->Result(),
                                                 cond->Result()};

                load = b.Call(ty.vec2u(), core::BuiltinFn::kSelect, args);
            }
        } else {
            TINT_IR_UNREACHABLE(ir);
        }
        return b.Bitcast(result_ty, load);
    }

    // Returns the instruction for getting a vector-of-f16 value of type `result_ty`
    // out of the pointer-to-vec4u value `access`. Get the components starting
    // at byte offset (byte_idx % 16).
    // A `vec4` or `vec3` of `f16` has 8-byte alignment, while a `vec2` of `f16` has 4-byte
    // alignment. So, this means we'll have memory like:
    //
    // Byte:     |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
    // Scalar Index:       0         |         1         |         2         |         3         |
    // vec4<u32>:|         x         |         y         |         z         |         w         |
    // vec4<f16>:|   x     |    y    |    z    |    w    |   x     |    y    |    z    |    w    |
    // vec3<f16>:|   x     |    y    |    z    |         |   x     |    y    |    z    |         |
    // vec2<f16>:|   x     |    y    |    x    |    y    |   x     |    y    |    x    |    y    |
    core::ir::Instruction* MakeVectorLoadF16(core::ir::Access* access,
                                             const core::type::Vector* result_ty,
                                             core::ir::Value* byte_idx) {
        // Vec4 ends up being the same as a bitcast of vec2<u32> to a vec4<f16>.
        // A vec3 will be stored as a vec4, so we can bitcast as if we're a vec4
        // and swizzle out the last element.
        if (result_ty->Width() == 3 || result_ty->Width() == 4) {
            core::ir::Instruction* load = nullptr;
            auto* vec_idx = CalculateVectorOffset(byte_idx);  // 0 or 2
            if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                if (cnst->Value()->ValueAs<uint32_t>() == 2u) {
                    load = b.Swizzle(ty.vec2u(), b.Load(access), {2, 3});
                } else {
                    load = b.Swizzle(ty.vec2u(), b.Load(access), {0, 1});
                }
            } else {
                auto* ubo = b.Load(access);
                // if vec_idx == 2 -> zw
                auto* sw_lhs = b.Swizzle(ty.vec2u(), ubo, {2, 3});
                // else -> xy
                auto* sw_rhs = b.Swizzle(ty.vec2u(), ubo, {0, 1});
                auto* cond = b.Equal(vec_idx, 2_u);
                auto args = Vector{sw_rhs->Result(), sw_lhs->Result(), cond->Result()};
                load = b.Call(ty.vec2u(), core::BuiltinFn::kSelect, std::move(args));
            }
            if (result_ty->Width() == 3) {
                auto* bc = b.Bitcast(ty.vec4(result_ty->Type()), load);
                return b.Swizzle(result_ty, bc, {0, 1, 2});
            }
            return b.Bitcast(result_ty, load);
        }

        // Vec2 ends up being the same as a bitcast u32 to vec2<f16>
        if (result_ty->Width() == 2) {
            core::ir::Instruction* load = nullptr;
            auto* vec_idx = CalculateVectorOffset(byte_idx);  // 0, 1, 2, or 3
            if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                const auto vec_idx_val = cnst->Value()->ValueAs<uint32_t>();
                load = b.Swizzle(ty.u32(), b.Load(access), {vec_idx_val});
            } else {
                load = b.Access(ty.u32(), b.Load(access), vec_idx);
            }
            return b.Bitcast(result_ty, load);
        }

        TINT_IR_UNREACHABLE(ir);
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

    void MakeStore(core::ir::Instruction* inst,
                   core::ir::Var* var,
                   const core::type::Type* type,
                   core::ir::Value* byte_idx,
                   core::ir::Value* value) {
        if (type->IsFloatScalar() || type->IsIntegerScalar()) {
            MakeScalarStore(var, type, byte_idx, value);
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
            [&](const core::type::Vector* v) { MakeVectorStore(var, v, byte_idx, value); },
            TINT_ICE_ON_NO_MATCH);
    }

    void MakeScalarStore(core::ir::Var* var,
                         const core::type::Type* type,
                         core::ir::Value* byte_idx,
                         core::ir::Value* value) {
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto as = var->Result()->Type()->As<core::type::Pointer>()->AddressSpace();
        auto* access = b.Access(ty.ptr(as, ty.vec4u()), var, array_idx);
        auto* vec_idx = CalculateVectorOffset(byte_idx);

        if (type->Is<core::type::F16>()) {
            auto* new_u32 = InsertScalarF16(access, vec_idx, byte_idx, value);
            b.StoreVectorElement(access, vec_idx, new_u32);
            return;
        }

        auto* val_u32 = b.Bitcast(ty.u32(), value);
        b.StoreVectorElement(access, vec_idx, val_u32);
    }

    void MakeVectorStore(core::ir::Var* var,
                         const core::type::Vector* vec_ty,
                         core::ir::Value* byte_idx,
                         core::ir::Value* value) {
        if (vec_ty->DeepestElement()->Is<core::type::F16>()) {
            auto* array_idx = OffsetValueToArrayIndex(byte_idx);
            auto as = var->Result()->Type()->As<core::type::Pointer>()->AddressSpace();
            auto* access = b.Access(ty.ptr(as, ty.vec4u()), var, array_idx);
            MakeVectorStoreF16(access, vec_ty, byte_idx, value);
            return;
        }

        // Decompose vector store into scalar stores
        for (uint32_t i = 0; i < vec_ty->Width(); ++i) {
            auto* elem_val = b.Access(vec_ty->Type(), value, u32(i))->Result();
            auto elem_size = vec_ty->Type()->Size();

            // Calculate offset for this element
            OffsetData od{i * elem_size, {byte_idx}};
            auto* elem_byte_idx = OffsetToValue(od);

            MakeScalarStore(var, vec_ty->Type(), elem_byte_idx, elem_val);
        }
    }

    // Stores a vector-of-f16 value into memory.
    // A `vec4` or `vec3` of `f16` occupies 8 bytes (stored in vec2<u32>).
    // A `vec2` of `f16` occupies 4 bytes (stored in a single u32).
    void MakeVectorStoreF16(core::ir::Access* access,
                            const core::type::Vector* vec_ty,
                            core::ir::Value* byte_idx,
                            core::ir::Value* value) {
        auto* vec_idx = CalculateVectorOffset(byte_idx);

        if (vec_ty->Width() == 4) {
            // vec4<f16> -> vec2<u32>
            auto* val_vec2u = b.Bitcast<vec2<u32>>(value);
            auto* val_x = b.Access<u32>(val_vec2u, 0_u)->Result();
            auto* val_y = b.Access<u32>(val_vec2u, 1_u)->Result();

            // Store the two u32 values at vec_idx and vec_idx+1
            if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                auto idx = cnst->Value()->ValueAs<uint32_t>();
                b.StoreVectorElement(access, u32(idx), val_x);
                b.StoreVectorElement(access, u32(idx + 1), val_y);
            } else {
                b.StoreVectorElement(access, vec_idx, val_x);
                auto* next_idx = b.Add(vec_idx, 1_u)->Result();
                b.StoreVectorElement(access, next_idx, val_y);
            }
            return;
        }

        if (vec_ty->Width() == 3) {
            // vec3<f16> is stored as vec4<f16>, so it also occupies 8 bytes (vec2<u32>)
            // Extend vec3 to vec4 with zero padding, then store as vec2<u32>
            auto* x = b.Access<f16>(value, 0_u)->Result();
            auto* y = b.Access<f16>(value, 1_u)->Result();
            auto* z = b.Access<f16>(value, 2_u)->Result();
            auto* vec4_val = b.Construct<vec4<f16>>(x, y, z, b.Value(f16(0)));
            auto* val_vec2u = b.Bitcast<vec2<u32>>(vec4_val);
            auto* val_x = b.Access<u32>(val_vec2u, 0_u)->Result();
            auto* val_y = b.Access<u32>(val_vec2u, 1_u)->Result();

            if (auto* cnst = vec_idx->As<core::ir::Constant>()) {
                auto idx = cnst->Value()->ValueAs<uint32_t>();
                b.StoreVectorElement(access, u32(idx), val_x);
                b.StoreVectorElement(access, u32(idx + 1), val_y);
            } else {
                b.StoreVectorElement(access, vec_idx, val_x);
                auto* next_idx = b.Add(vec_idx, 1_u)->Result();
                b.StoreVectorElement(access, next_idx, val_y);
            }
            return;
        }

        if (vec_ty->Width() == 2) {
            // vec2<f16> -> u32
            auto* val_u32 = b.Bitcast<u32>(value);
            b.StoreVectorElement(access, vec_idx, val_u32);
            return;
        }

        TINT_IR_UNREACHABLE(ir);
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

Result<SuccessType> DecomposeUniformAccess(core::ir::Module& ir) {
    auto result =
        ValidateAndDumpIfNeeded(ir, "core.DecomposeUniformAccess",
                                core::ir::Capabilities{
                                    core::ir::Capability::kAllow8BitIntegers,
                                    core::ir::Capability::kAllowClipDistancesOnF32ScalarAndVector,
                                    core::ir::Capability::kAllowDuplicateBindings,
                                    core::ir::Capability::kAllowNonCoreTypes,
                                });
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process(core::AddressSpace::kUniform);

    return Success;
}

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

    State{ir}.Process(core::AddressSpace::kWorkgroup);

    return Success;
}

}  // namespace tint::core::ir::transform
