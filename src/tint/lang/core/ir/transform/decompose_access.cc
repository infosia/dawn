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

#include "src/tint/lang/core/ir/transform/decompose_access.h"

#include <algorithm>
#include <limits>
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

    /// The transform options.
    const DecomposeAccessOptions& options;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    using VarTypePair = std::pair<core::ir::Var*, const core::type::Type*>;
    /// Maps a variable and type to the load function
    Hashmap<VarTypePair, core::ir::Function*, 2> var_and_type_to_load_fn_{};
    /// Maps a variable and type to the store function
    Hashmap<VarTypePair, core::ir::Function*, 2> var_and_type_to_store_fn_{};

    const type::Type* base_ty_ = nullptr;
    const type::Pointer* base_ptr_ty_ = nullptr;
    bool module_contains_f16_ = false;

    /// Process the module.
    void Process() {
        // First pass: detect if module contains any f16 types
        for (auto* inst : *ir.root_block) {
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }
            auto* var_ty = var->Result()->Type()->As<core::type::Pointer>();
            if (!var_ty) {
                continue;
            }
            if (ContainsF16Type(var_ty->StoreType())) {
                module_contains_f16_ = true;
                break;
            }
        }

        Vector<core::ir::Var*, 4> var_worklist;
        for (auto* inst : *ir.root_block) {
            // Allow this to run before or after PromoteInitializers by handling non-var root_block
            // entries
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }

            // DecomposeStorageAccess maybe have converted the var pointers into ByteAddressBuffer
            // objects. Since they've been changed, then they're Storage buffers and we don't care
            // about them here.
            auto* var_ty = var->Result()->Type()->As<core::type::Pointer>();
            if (!var_ty) {
                continue;
            }

            // Always decompose buffer types, otherwise depend on the options.
            if (var_ty->StoreType()->Is<type::Buffer>()) {
                var_worklist.Push(var);
            } else if ((var_ty->AddressSpace() == AddressSpace::kStorage && options.storage) ||
                       (var_ty->AddressSpace() == AddressSpace::kUniform && options.uniform) ||
                       (var_ty->AddressSpace() == AddressSpace::kWorkgroup && options.workgroup)) {
                // Atomics could be handled in some backends, but not in a generic way.
                // Bool is spec'd as same as u32 so that could also be handled.
                // TODO(b/474819671): Should bool be supported? WebGPU issue 5338.
                if (!ContainsAtomicOrBool(var_ty->StoreType())) {
                    var_worklist.Push(var);
                }
            }
        }

        for (auto* var : var_worklist) {
            auto* result = var->Result();
            SetBaseEleType(var);

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
                    [&](core::ir::StoreVectorElement* s) { StoreVectorElement(s, var, od); },
                    [&](core::ir::Load* l) { Load(l, var, od); },
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
                    [&](core::ir::CoreBuiltinCall* call) {
                        switch (call->Func()) {
                            case core::BuiltinFn::kArrayLength:
                                ArrayLength(call, var, var_ty->StoreType(), {});
                                break;
                            case core::BuiltinFn::kBufferLength:
                                BufferLength(call, var, var_ty->StoreType());
                                break;
                            case core::BuiltinFn::kBufferView:
                                BufferView(call, var, var_ty->StoreType(), {});
                                break;
                            default:
                                TINT_IR_UNREACHABLE(ir);
                        }
                    },
                    TINT_ICE_ON_NO_MATCH);
            }

            auto HasRuntimeSize = [&](const type::Type* type) {
                return tint::Switch(
                    type,
                    [&](const type::Array* arr) {
                        return arr->Count()->Is<type::RuntimeArrayCount>();
                    },
                    [&](const type::Buffer* buf) {
                        return buf->Count()->Is<type::RuntimeArrayCount>();
                    },
                    [&](const type::Struct* str) {
                        auto* last =
                            str->Element(static_cast<uint32_t>(str->Members().Length()) - 1);
                        if (auto* arr = last->As<type::Array>()) {
                            return arr->Count()->Is<type::RuntimeArrayCount>();
                        }
                        return false;
                    },
                    [&](Default) { return false; });
            };

            // Swap the result type of the `var` to the new result type
            const type::Array* array_ty = nullptr;
            if (HasRuntimeSize(var_ty->StoreType())) {
                // Use a runtime-sized array of the base type.
                array_ty = ty.runtime_array(BaseEleType());
            } else {
                auto array_length = NumBaseElements(var_ty->StoreType());
                array_ty = ty.array(BaseEleType(), array_length);
            }
            result->SetType(ty.ptr(var_ty->AddressSpace(), array_ty, var_ty->Access()));
        }
    }

    const type::Type* BaseEleType() { return base_ty_; }

    const type::Pointer* BaseEleTypePtr() { return base_ptr_ty_; }

    // Returns the number of BaseEleType elements need to represent `type` rounded up.
    uint32_t NumBaseElements(const type::Type* type) {
        return (type->Size() + BaseEleType()->Size() - 1) / BaseEleType()->Size();
    }

    // OffsetData represents an unsigned integer expression.
    // The value is the sum of a const part, held in `byte_offset`, and
    // non-const parts in `byte_offset_expr`.
    struct OffsetData {
        uint32_t byte_offset = 0;
        Vector<core::ir::Value*, 4> byte_offset_expr{};
    };

    bool ContainsAtomicOrBool(const type::Type* type) const {
        return tint::Switch(
            type,  //
            [&](const type::Bool*) { return true; }, [&](const type::Atomic*) { return true; },
            [&](const type::Vector* vec_ty) { return ContainsAtomicOrBool(vec_ty->Type()); },
            [&](const type::Array* array_ty) { return ContainsAtomicOrBool(array_ty->ElemType()); },
            [&](const type::Struct* struct_ty) {
                for (auto* member : struct_ty->Members()) {
                    if (ContainsAtomicOrBool(member->Type())) {
                        return true;
                    }
                }
                return false;
            },
            [&](Default) { return false; });
    }

    bool ContainsF16Type(const type::Type* type) const {
        return tint::Switch(
            type, [&](const type::F16*) { return true; },
            [&](const type::Vector* vec_ty) { return ContainsF16Type(vec_ty->Type()); },
            [&](const type::Matrix* mat_ty) {
                return ContainsF16Type(mat_ty->ColumnType()->Type());
            },
            [&](const type::Array* array_ty) { return ContainsF16Type(array_ty->ElemType()); },
            [&](const type::Struct* struct_ty) {
                for (auto* member : struct_ty->Members()) {
                    if (ContainsF16Type(member->Type())) {
                        return true;
                    }
                }
                return false;
            },
            [&](Default) { return false; });
    }

    uint32_t SmallestElementSize(const type::Type* type) {
        return tint::Switch(
            type,  //
            [&](const type::Scalar* scalar) { return scalar->Size(); },
            [&](const type::Vector* vector) {
                // if (vector->Width() == 3) {
                //     return vector->Type()->Size();
                // }
                return vector->Type()->Size();
            },
            [&](const type::Matrix* matrix) { return SmallestElementSize(matrix->ColumnType()); },
            [&](const type::Array* array) { return SmallestElementSize(array->ElemType()); },
            [&](const type::Struct* str) {
                uint32_t size = std::numeric_limits<uint32_t>::max();
                for (auto* member : str->Members()) {
                    size = std::min(size, SmallestElementSize(member->Type()));
                }
                return size;
            },
            TINT_ICE_ON_NO_MATCH);
    }

    // Returns the smallest access size (load/store) in bytes for `var` up to a max of `16` bytes.
    uint32_t SmallestAccessSize(core::ir::Var* var) {
        uint32_t size = std::numeric_limits<uint32_t>::max();
        auto worklist = var->Result()->UsagesSorted();
        while (!worklist.IsEmpty()) {
            auto usage = worklist.Pop();
            auto* use = usage.instruction;

            size = std::min(
                size,
                tint::Switch(
                    use,
                    [&](core::ir::LoadVectorElement* lve) { return lve->Result()->Type()->Size(); },
                    [&](core::ir::StoreVectorElement* sve) { return sve->Value()->Type()->Size(); },
                    [&](core::ir::Load* load) {
                        return SmallestElementSize(load->Result()->Type());
                    },
                    [&](core::ir::Store* store) {
                        return SmallestElementSize(store->From()->Type());
                    },
                    [&](core::ir::Instruction* inst) {
                        bool bufferView = false;
                        if (auto* call = inst->As<core::ir::CoreBuiltinCall>()) {
                            bufferView = call->Func() == core::BuiltinFn::kBufferView;
                        }
                        if (inst->IsAnyOf<core::ir::Access, core::ir::Let>() || bufferView) {
                            for (auto u : inst->Result()->UsagesSorted()) {
                                worklist.Push(u);
                            }
                        }
                        if (auto* call = inst->As<core::ir::CoreBuiltinCall>()) {
                            if (call->Func() == core::BuiltinFn::kArrayLength) {
                                auto* ptr_ty = call->Args()[0]->Type()->As<type::Pointer>();
                                return SmallestElementSize(ptr_ty->StoreType());
                            }
                        }
                        return size;
                    }));
        }
        // No need to be larger than a vec4u.
        return std::min(size, 16u);
    }

    // Sets the base type based on whether the module contains f16 types.
    // If module has f16: use vec4<f16> for all variables.
    // Otherwise: use vec4<u32> for all variables.
    void SetBaseEleType(core::ir::Var* var) {
        auto* var_ty = var->Result()->Type()->As<core::type::Pointer>();

        if (module_contains_f16_) {
            base_ty_ = ty.vec4(ty.f16());
        } else {
            base_ty_ = ty.vec4u();
        }

        base_ptr_ty_ = ty.ptr(var_ty->AddressSpace(), base_ty_, var_ty->Access());
    }

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

    // Converts a byte offset to an index into an array of base type.
    // After the transform runs the store type of the uniform buffer variable is an
    // array of vec4u.
    // Note, this must be called inside a builder insert block (Append, InsertBefore, etc)
    core::ir::Value* OffsetValueToArrayIndex(core::ir::Value* val) {
        if (auto* cnst = val->As<core::ir::Constant>()) {
            auto v = cnst->Value()->ValueAs<uint32_t>();
            return b.Value(u32(v / BaseEleType()->Size()));
        }
        return b.Divide(val, u32(BaseEleType()->Size()))->Result();
    }

    // Calculates the index of the vector element containing the byte at (byte_idx %
    // src_ty->Size()). Assumes the upper bits of byte_idx have already been used to access the
    // correct vector array element in the underlying variable.
    core::ir::Value* CalculateVectorOffset(core::ir::Value* byte_idx, const type::Vector* src_ty) {
        if (auto* byte_cnst = byte_idx->As<core::ir::Constant>()) {
            return b.Value(u32((byte_cnst->Value()->ValueAs<uint32_t>() % src_ty->Size()) /
                               src_ty->Type()->Size()));
        }
        // Note: Using bitwise-and and shift instead of modulo and divide here was necessary to
        // avoid an FXC miscompile. See https://crbug.com/454366353.
        return b
            .ShiftRight(b.And(byte_idx, b.Constant(u32(src_ty->Size() - 1))),
                        u32(log2(src_ty->Type()->Size())))
            ->Result();
    }

    // Note, must be called inside a builder insert block (Append, InsertBefore, etc)
    void UpdateOffsetData(core::ir::Value* v, uint32_t elm_size, OffsetData* offset) {
        tint::Switch(
            v,  //
            [&](core::ir::Constant* idx_value) {
                offset->byte_offset += idx_value->Value()->ValueAs<uint32_t>() * elm_size;
            },
            [&](core::ir::Value* val) {
                auto* idx = val;
                idx = b.InsertConvertIfNeeded(ty.u32(), val);
                offset->byte_offset_expr.Push(b.Multiply(idx, u32(elm_size))->Result());
            },
            TINT_ICE_ON_NO_MATCH);
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

        for (auto* idx_value : a->Indices()) {
            tint::Switch(
                obj_ty,
                [&](const core::type::Vector* v) {
                    b.InsertBefore(
                        a, [&] { UpdateOffsetData(idx_value, v->Type()->Size(), &offset); });
                    obj_ty = v->Type();
                },
                [&](const core::type::Matrix* m) {
                    b.InsertBefore(
                        a, [&] { UpdateOffsetData(idx_value, m->ColumnStride(), &offset); });
                    obj_ty = m->ColumnType();
                },
                [&](const core::type::Array* ary) {
                    b.InsertBefore(
                        a, [&] { UpdateOffsetData(idx_value, ary->ImplicitStride(), &offset); });
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

        AccessUses(a, var, obj_ty, offset);
    }

    void BufferView(core::ir::CoreBuiltinCall* call,
                    core::ir::Var* var,
                    const type::Type* obj_type,
                    OffsetData offset) {
        auto* offset_arg = call->Args()[1];
        b.InsertBefore(call, [&] { UpdateOffsetData(offset_arg, 1, &offset); });
        obj_type = call->Result()->Type()->As<type::Pointer>()->StoreType();

        AccessUses(call, var, obj_type, offset);
    }

    void AccessUses(core::ir::Instruction* inst,
                    core::ir::Var* var,
                    const type::Type* obj_ty,
                    OffsetData offset) {
        auto usages = inst->Result()->UsagesSorted();
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
                    let->Result()->ReplaceAllUsesWith(inst->Result());
                    let->Destroy();
                },
                [&](core::ir::Access* sub_access) {
                    // Treat an access chain of the access chain as a continuation of the outer
                    // chain. Pass through the object we stopped at and the current byte_offset
                    // and then restart the access chain replacement for the new access chain.
                    Access(sub_access, var, obj_ty, offset);
                },
                [&](core::ir::Load* ld) {
                    inst->Result()->RemoveUsage(usage);
                    Load(ld, var, offset);
                },
                [&](core::ir::Store* st) {
                    inst->Result()->RemoveUsage(usage);
                    Store(st, var, offset);
                },
                [&](core::ir::LoadVectorElement* lve) {
                    inst->Result()->RemoveUsage(usage);
                    LoadVectorElement(lve, var, offset);
                },
                [&](core::ir::StoreVectorElement* sve) {
                    inst->Result()->RemoveUsage(usage);
                    StoreVectorElement(sve, var, offset);
                },
                [&](core::ir::CoreBuiltinCall* call) {
                    // Note: we don't need to check for bufferView and bufferLength as they cannot
                    // be encountered after an access.
                    if (call->Func() == core::BuiltinFn::kArrayLength) {
                        ArrayLength(call, var, obj_ty, offset);
                    }
                },
                TINT_ICE_ON_NO_MATCH);
        }
        inst->Destroy();
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

    // Creates the appropriate load instructions for the given result type.
    core::ir::Instruction* MakeLoad(core::ir::Instruction* inst,
                                    core::ir::Var* var,
                                    const core::type::Type* result_ty,
                                    core::ir::Value* byte_idx) {
        // TODO(b/474819671): Update if we support bool.
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

    // Returns a vector of `num_loads` load instructions of BaseEleType from `var` starting from
    // `start_idx`.
    Vector<Instruction*, 4> MakeNLoadInsts(Var* var, Value* start_idx, uint32_t num_loads) {
        Vector<Instruction*, 4> loads;
        auto* idx = start_idx;
        for (uint32_t i = 0; i < num_loads; i++) {
            if (i > 0) {
                if (auto* cnst = idx->As<Constant>()) {
                    idx = b.Constant(u32(cnst->Value()->ValueAs<uint32_t>() + 1));
                } else {
                    idx = b.Add(idx, b.Constant(1_u))->Result();
                }
            }
            auto* access = b.Access(BaseEleTypePtr(), var, idx);
            loads.Push(b.Load(access));
        }
        return loads;
    }

    // Returns a vector of `num_loads` load values of BaseEleType from `var` starting from
    // `start_idx`.
    Vector<Value*, 4> MakeNLoads(Var* var, Value* start_idx, uint32_t num_loads) {
        auto insts = MakeNLoadInsts(var, start_idx, num_loads);
        Vector<Value*, 4> vals;
        vals.Reserve(insts.Length());
        for (auto* inst : insts) {
            vals.Push(inst->Result());
        }
        return vals;
    }

    core::ir::Instruction* MakeScalarLoad(core::ir::Var* var,
                                          const core::type::Type* result_ty,
                                          core::ir::Value* byte_idx) {
        // Base type is always vec4<f16> or vec4<u32>, both are 16 bytes.
        // Scalars are always smaller than vec4, so we load one vec4 and extract.
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto* access = b.Access(BaseEleTypePtr(), var, array_idx);

        // BaseEleType is always a vec4
        auto* vec_ty = BaseEleType()->As<type::Vector>();
        TINT_IR_ASSERT(ir, vec_ty && vec_ty->Width() == 4);

        auto* vec_idx = CalculateVectorOffset(byte_idx, vec_ty);
        auto* load = b.LoadVectorElement(access, vec_idx);
        auto* elem_ty = load->Result()->Type();

        // Handle size conversion
        if (result_ty->Size() == elem_ty->Size()) {
            // Same size, direct bitcast if needed
            return b.InsertBitcastIfNeeded(result_ty, load);
        } else if (result_ty->Size() > elem_ty->Size()) {
            // Larger result (e.g., f16 -> u32): need to load multiple elements
            uint32_t ratio = result_ty->Size() / elem_ty->Size();
            auto* vec_result_ty = ty.vec(elem_ty, ratio);

            // Load multiple consecutive elements
            Vector<Value*, 4> elements;
            for (uint32_t i = 0; i < ratio; i++) {
                Value* idx = vec_idx;
                if (i > 0) {
                    if (auto* cnst = vec_idx->As<Constant>()) {
                        idx = b.Constant(u32(cnst->Value()->ValueAs<uint32_t>() + i));
                    } else {
                        idx = b.Add(vec_idx, u32(i))->Result();
                    }
                }
                if (i == 0) {
                    elements.Push(load->Result());
                } else {
                    elements.Push(b.LoadVectorElement(access, idx)->Result());
                }
            }

            auto* vec_construct = b.Construct(vec_result_ty, elements);
            return b.Bitcast(result_ty, vec_construct);
        } else {
            // Smaller result (e.g., u32 -> u16): bitcast and extract lower bits
            // This happens when base type is vec4<u32> but loading u16 or f16
            uint32_t ratio = elem_ty->Size() / result_ty->Size();
            auto* smaller_vec_ty = ty.vec(result_ty, ratio);
            auto* cast = b.Bitcast(smaller_vec_ty, load);

            // Extract the appropriate element based on byte offset within the u32
            Value* element_idx = nullptr;
            if (auto* cnst = byte_idx->As<Constant>()) {
                uint32_t byte_offset = cnst->Value()->ValueAs<uint32_t>();
                uint32_t elem_offset_in_u32 = (byte_offset % elem_ty->Size()) / result_ty->Size();
                element_idx = b.Constant(u32(elem_offset_in_u32));
            } else {
                // Dynamic index: (byte_idx % elem_ty->Size()) / result_ty->Size()
                auto* mod = b.Modulo(byte_idx, u32(elem_ty->Size()));
                element_idx = b.Divide(mod, u32(result_ty->Size()))->Result();
            }

            return b.Access(result_ty, cast, element_idx);
        }
    }

    // Extract a 2-byte element from a vec4 load result.
    // This is used when loading f16 or u16 from a vec4<f16> base type.
    core::ir::Access* ExtractScalar2Bytes(core::ir::Instruction* load,
                                          const type::Type* result_ty,
                                          core::ir::Value* byte_idx) {
        // Base type is vec4<f16> (16 bytes), extracting 2-byte element.
        // Result type is 2 bytes, so we can extract from the vec4 directly.
        TINT_IR_ASSERT(ir, result_ty->Size() == 2);
        TINT_IR_ASSERT(ir, load->Result()->Type()->Is<type::Vector>());

        const uint32_t load_size = load->Result()->Type()->Size();
        uint32_t num_eles = load_size / result_ty->Size();
        const type::Type* vec_ty = ty.vec(result_ty, num_eles);
        core::ir::Value* element_index = nullptr;
        if (auto* cnst = byte_idx->As<core::ir::Constant>()) {
            if (cnst->Value()->ValueAs<uint32_t>() % 4 == 0) {
                element_index = b.Constant(0_u);
            } else {
                element_index = b.Constant(1_u);
            }
        } else {
            auto* false_ = b.Value(1_u);
            auto* true_ = b.Value(0_u);
            auto* cond = b.Equal(b.Modulo(byte_idx, 4_u), 0_u);

            Vector<core::ir::Value*, 3> args{false_, true_, cond->Result()};
            element_index = b.Call(ty.u32(), core::BuiltinFn::kSelect, args)->Result();
        }

        auto* bitcast = b.Bitcast(vec_ty, load);
        return b.Access(result_ty, bitcast, element_index);
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
        if (result_ty->DeepestElement()->Size() == 2) {
            return MakeVectorLoadF16(var, result_ty, byte_idx);
        }

        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        uint32_t num_loads = NumBaseElements(result_ty);
        auto loads = MakeNLoadInsts(var, array_idx, num_loads);

        // Base type is always vec4 (vec4<f16> or vec4<u32>)
        TINT_IR_ASSERT(ir, loads[0]->Result()->Type()->Is<type::Vector>());
        TINT_IR_ASSERT(ir, loads[0]->Result()->Type()->As<type::Vector>()->Width() == 4);

        Instruction* value = nullptr;
        if (loads.Length() == 1) {
            value = loads[0];
        } else {
            // Multiple vec4 loads needed for larger types
            Vector<Value*, 4> construct_args;
            for (auto* l : loads) {
                construct_args.Push(l->Result());
            }
            // This shouldn't happen with vec4 base type for standard vector types
            TINT_IR_UNREACHABLE(ir);
        }

        TINT_IR_ASSERT(ir, result_ty->DeepestElement()->Size() == 4);

        // Extract the appropriate elements from the vec4
        core::ir::Instruction* load = nullptr;
        auto* base_vec_ty = BaseEleType()->As<type::Vector>();
        auto result_vec_ty = ty.vec(base_vec_ty->Type(), result_ty->Width());

        if (result_ty->Width() == 4) {
            load = value;
        } else if (result_ty->Width() == 3) {
            load = b.Swizzle(result_vec_ty, value, {0, 1, 2});
        } else if (result_ty->Width() == 2) {
            load = b.Swizzle(result_vec_ty, value, {0, 1});
        } else {
            TINT_IR_UNREACHABLE(ir);
        }
        return b.InsertBitcastIfNeeded(result_ty, load);
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
    core::ir::Instruction* MakeVectorLoadF16(core::ir::Var* var,
                                             const core::type::Vector* result_ty,
                                             core::ir::Value* byte_idx) {
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto loads = MakeNLoads(var, array_idx, 1);

        // Base type is always vec4<f16> or vec4<u32>
        auto* base_vec_ty = BaseEleType()->As<type::Vector>();
        TINT_IR_ASSERT(ir, base_vec_ty && base_vec_ty->Width() == 4);
        TINT_IR_ASSERT(ir, loads.Length() == 1);

        // Base type is vec4<f16> or vec4<u32>, result is vector of 2-byte elements
        // Simply bitcast from vec4 base and extract needed elements
        auto* base_as_result_ty = ty.vec(result_ty->Type(), 4);
        auto* bitcast = b.Bitcast(base_as_result_ty, loads[0]);

        if (result_ty->Width() == 4) {
            return bitcast;
        } else if (result_ty->Width() == 3) {
            return b.Swizzle(result_ty, bitcast, {0, 1, 2});
        } else if (result_ty->Width() == 2) {
            return b.Swizzle(result_ty, bitcast, {0, 1});
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

    // Updates an arrayLength call to account for `offset`.
    // The builtin call returns the length of the whole variable. The array type may have changed
    // too, so we must account for that difference. Any offset must be subtracted from that value to
    // provide the same result as before the transform. Offsets can come from two sources:
    //
    // 1. An offset from an access into a structure.
    // 2. An offset (possibly runtime value) from a bufferView call.
    void ArrayLength(core::ir::CoreBuiltinCall* call,
                     core::ir::Var* var,
                     const type::Type* type,
                     OffsetData offset) {
        auto* array_ty = type->As<type::Array>();
        TINT_IR_ASSERT(ir, array_ty && array_ty->Count()->Is<core::type::RuntimeArrayCount>());

        // The arrayLength of the transformed variable will always be a multiple of the base type.
        TINT_IR_ASSERT(ir, array_ty->ElemType()->Size() / BaseEleType()->Size() >= 1);
        const uint32_t ratio = array_ty->ImplicitStride() / BaseEleType()->Size();

        // Given:
        // b = bufferView var, offset1
        // a = access b, offset2
        // l = arrayLength a
        //
        // Transformed:
        // l = arrayLength var
        // o = add offset1, bytes(offset2)
        // s = sub l, o
        // d = div s, ratio
        b.InsertBefore(call, [&] {
            // Re-create the arrayLength call to simplify RAUW below.
            auto* len = b.Call(ty.u32(), BuiltinFn::kArrayLength, var);

            Value* value = nullptr;
            ir::Instruction* inst = len;
            // bufferView calls may have introduced a single runtime offset value.
            TINT_IR_ASSERT(ir, offset.byte_offset_expr.Length() <= 1);
            if (offset.byte_offset > 0 && offset.byte_offset_expr.Length() > 0) {
                value = b.Add(offset.byte_offset_expr[0], u32(offset.byte_offset))->Result();
            } else if (offset.byte_offset_expr.Length() > 0) {
                value = offset.byte_offset_expr[0];
            } else if (offset.byte_offset > 0) {
                value = b.Constant(u32(offset.byte_offset / BaseEleType()->Size()));
            }
            if (value && offset.byte_offset_expr.Length() > 0) {
                value = b.Divide(value, u32(BaseEleType()->Size()))->Result();
            }
            if (value) {
                inst = b.Subtract(inst, value);
            }
            if (ratio != 1u) {
                inst = b.Divide(inst, u32(ratio));
            }
            call->Result()->ReplaceAllUsesWith(inst->Result());
        });
        call->Destroy();
    }

    // bufferLength calls are replaced with arrayLength calls for the entire var size with two
    // special cases:
    //
    // 1. The bufferLength has a third operand added by DirectVariableAccess that represents the
    //    lowest limit encountered.
    // 2. The buffer is sized and we can use that directly from the type.
    void BufferLength(core::ir::CoreBuiltinCall* call, core::ir::Var* var, const type::Type* type) {
        auto* buffer_ty = type->As<core::type::Buffer>();
        TINT_IR_ASSERT(ir, buffer_ty && (buffer_ty->Count()->Is<type::RuntimeArrayCount>() ||
                                         buffer_ty->Count()->Is<type::ConstantArrayCount>()));

        if (call->Args().Length() > 1) {
            // Direct variable access encoded a lower limit.
            call->Result()->ReplaceAllUsesWith(call->Args()[1]);
        } else if (auto* cnst = buffer_ty->Count()->As<type::ConstantArrayCount>()) {
            call->Result()->ReplaceAllUsesWith(b.Constant(u32(cnst->value)));
        } else {
            TINT_IR_ASSERT(ir, buffer_ty->Count()->Is<type::RuntimeArrayCount>());
            b.InsertBefore(call, [&] {
                // arrayLength(var) * BaseEleType()->Size()
                ir::Instruction* inst = b.Call(ty.u32(), core::BuiltinFn::kArrayLength, var);
                inst = b.Multiply(inst, u32(BaseEleType()->Size()));
                call->Result()->ReplaceAllUsesWith(inst->Result());
            });
        }
        call->Destroy();
    }

    // Store the scalar value `from` at `byte_idx` offset.
    //
    // Base type is vec4<f16> or vec4<u32>. We need to store a scalar value
    // to one element of the vec4.
    void MakeScalarStore(core::ir::Var* var, core::ir::Value* from, core::ir::Value* byte_idx) {
        // Base type is always vec4<f16> or vec4<u32> (16 bytes)
        // Scalars are always smaller, so we store to a single element of vec4
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);
        auto* access = b.Access(BaseEleTypePtr(), var, array_idx);

        auto* base_vec_ty = BaseEleType()->As<type::Vector>();
        TINT_IR_ASSERT(ir, base_vec_ty && base_vec_ty->Width() == 4);

        auto* vec_idx = CalculateVectorOffset(byte_idx, base_vec_ty);
        auto* elem_ty = base_vec_ty->Type();

        // Handle size conversion
        if (from->Type()->Size() == elem_ty->Size()) {
            // Same size, direct bitcast if needed
            auto* value = b.InsertBitcastIfNeeded(elem_ty, from);
            b.StoreVectorElement(access, vec_idx, value);
        } else if (from->Type()->Size() > elem_ty->Size()) {
            // Larger source (e.g., u32 -> f16): bitcast to vector and store all elements
            uint32_t ratio = from->Type()->Size() / elem_ty->Size();
            auto* vec_ty = ty.vec(elem_ty, ratio);
            auto* cast = b.Bitcast(vec_ty, from);

            // Store each element
            for (uint32_t i = 0; i < ratio; i++) {
                auto* element = b.Access(elem_ty, cast, u32(i))->Result();
                Value* idx = vec_idx;
                if (i > 0) {
                    if (auto* cnst = vec_idx->As<Constant>()) {
                        idx = b.Constant(u32(cnst->Value()->ValueAs<uint32_t>() + i));
                    } else {
                        idx = b.Add(vec_idx, u32(i))->Result();
                    }
                }
                b.StoreVectorElement(access, idx, element);
            }
        } else {
            // Smaller source (e.g., u16 -> u32): need to pack into larger type
            // This happens when base type is vec4<u32> but storing u16 or f16
            // We need to load the current u32, replace the appropriate bytes, and write back

            // Load the current u32 value
            auto* current = b.LoadVectorElement(access, vec_idx);

            // Determine which 2-byte half of the u32 to modify
            uint32_t ratio = elem_ty->Size() / from->Type()->Size();
            auto* vec_ty = ty.vec(from->Type(), ratio);

            // Bitcast current u32 to vec of smaller type (e.g., vec2<u16>)
            auto* cast_current = b.Bitcast(vec_ty, current);

            // Calculate which element in the vec to replace
            Value* element_idx = nullptr;
            if (auto* cnst = byte_idx->As<Constant>()) {
                uint32_t byte_offset = cnst->Value()->ValueAs<uint32_t>();
                uint32_t elem_offset_in_u32 =
                    (byte_offset % elem_ty->Size()) / from->Type()->Size();
                element_idx = b.Constant(u32(elem_offset_in_u32));
            } else {
                // Dynamic index: (byte_idx % elem_ty->Size()) / from->Type()->Size()
                auto* mod = b.Modulo(byte_idx, u32(elem_ty->Size()));
                element_idx = b.Divide(mod, u32(from->Type()->Size()))->Result();
            }

            // Build a new vector with the replaced element
            Vector<Value*, 4> elements;
            for (uint32_t i = 0; i < ratio; i++) {
                auto* idx = b.Constant(u32(i));
                auto* elem = b.Access(from->Type(), cast_current, idx)->Result();
                elements.Push(elem);
            }

            // Replace the target element with the new value
            // We need to use swizzle to reconstruct the vector
            auto* value = b.InsertBitcastIfNeeded(from->Type(), from);

            // For now, handle common cases (ratio = 2)
            TINT_IR_ASSERT(ir, ratio == 2);  // u32 = 2 x u16 or 2 x f16

            Value* new_vec = nullptr;
            if (auto* cnst_idx = element_idx->As<Constant>()) {
                uint32_t idx_val = cnst_idx->Value()->ValueAs<uint32_t>();
                if (idx_val == 0) {
                    // Replace first element: (value, elem[1])
                    new_vec = b.Construct(vec_ty, value, elements[1])->Result();
                } else {
                    // Replace second element: (elem[0], value)
                    new_vec = b.Construct(vec_ty, elements[0], value)->Result();
                }
            } else {
                // Dynamic selection - use select
                auto* vec_with_first = b.Construct(vec_ty, value, elements[1]);
                auto* vec_with_second = b.Construct(vec_ty, elements[0], value);
                auto* cond = b.Equal(element_idx, 0_u);
                new_vec = b.Call(vec_ty, core::BuiltinFn::kSelect,
                                 Vector{vec_with_second->Result(), vec_with_first->Result(),
                                        cond->Result()})
                              ->Result();
            }

            // Bitcast back to elem_ty (u32) and store
            auto* modified = b.Bitcast(elem_ty, new_vec);
            b.StoreVectorElement(access, vec_idx, modified);
        }
    }

    // Store the vector `from` at `byte_idx` offset.
    //
    // The two main cases to consider are:
    //
    // 1. The base type is a vector.
    //    i.  If a single store is needed, we simply cast the result if necessary
    //    ii. Multiple stores are needed, break the vector in sub-vectors and store those.
    // 2. The base type is a scalar. Store each element at successive indices.
    void MakeVectorStore(core::ir::Var* var, core::ir::Value* from, core::ir::Value* byte_idx) {
        auto* st_ty = from->Type()->As<type::Vector>();
        // Number of array elements need to store the scalar.
        auto num_array_eles = NumBaseElements(st_ty);
        auto* array_idx = OffsetValueToArrayIndex(byte_idx);

        // Base type is always vec4<f16> or vec4<u32> (16 bytes)
        // For vectors, we need to store them into vec4 elements
        TINT_IR_ASSERT(ir, num_array_eles == 1);

        auto* base_vec_ty = BaseEleType()->As<type::Vector>();
        TINT_IR_ASSERT(ir, base_vec_ty && base_vec_ty->Width() == 4);

        // Construct a vec4 with appropriate padding for vec2/vec3
        Value* value = nullptr;
        if (st_ty->Width() == 4) {
            value = b.InsertBitcastIfNeeded(BaseEleType(), from);
        } else if (st_ty->Width() == 3) {
            // vec3 -> vec4 with padding
            auto result_vec_ty = ty.vec(base_vec_ty->Type(), 3);
            auto* cast_val = b.InsertBitcastIfNeeded(result_vec_ty, from);
            value = b.Construct(BaseEleType(), cast_val, b.Zero(base_vec_ty->Type()))->Result();
        } else if (st_ty->Width() == 2) {
            // vec2 -> vec4 with padding
            auto result_vec_ty = ty.vec(base_vec_ty->Type(), 2);
            auto* cast_val = b.InsertBitcastIfNeeded(result_vec_ty, from);
            value = b.Construct(BaseEleType(), cast_val, b.Zero(base_vec_ty->Type()),
                                b.Zero(base_vec_ty->Type()))
                        ->Result();
        } else {
            TINT_IR_UNREACHABLE(ir);
        }

        auto* access = b.Access(BaseEleTypePtr(), var, array_idx);
        b.Store(access, value);
    }

    // Generate a store of `from` at `byte_idx` for the appropriate type.
    void MakeStore(core::ir::Instruction* inst,
                   core::ir::Var* var,
                   core::ir::Value* from,
                   core::ir::Value* byte_idx) {
        // TODO(b/474819671): Update if we support bool.
        if (from->Type()->IsFloatScalar() || from->Type()->IsIntegerScalar()) {
            MakeScalarStore(var, from, byte_idx);
            return;
        }

        tint::Switch(
            from->Type(),  //
            [&](const type::Struct* s) {
                auto* fn = GetStoreFunctionFor(inst, var, s);
                b.Call(fn, byte_idx, from);
            },
            [&](const type::Matrix* m) {
                auto* fn = GetStoreFunctionFor(inst, var, m);
                b.Call(fn, byte_idx, from);
            },
            [&](const type::Array* a) {
                auto* fn = GetStoreFunctionFor(inst, var, a);
                b.Call(fn, byte_idx, from);
            },
            [&](const type::Vector*) { MakeVectorStore(var, from, byte_idx); },
            TINT_ICE_ON_NO_MATCH);
    }

    void Store(core::ir::Store* s, core::ir::Var* var, OffsetData offset) {
        b.InsertBefore(s, [&] {
            auto* byte_idx = OffsetToValue(offset);
            MakeStore(s, var, s->From(), byte_idx);
        });
        s->Destroy();
    }

    void StoreVectorElement(core::ir::StoreVectorElement* s,
                            core::ir::Var* var,
                            OffsetData offset) {
        b.InsertBefore(s, [&] {
            UpdateOffsetData(s->Index(), s->Value()->Type()->Size(), &offset);
            auto* byte_idx = OffsetToValue(offset);
            MakeStore(s, var, s->Value(), byte_idx);
        });
        s->Destroy();
    }

    // Create a store function for the given `var` and `struct` combination. Essentially creates a
    // function similar to:
    //
    // fn custom_store_S(start_offset:u32, object:S) {
    //   store object at (start_offset + member 0 offset)
    //   store object at (start_offset + member 1 offset)
    //   ...
    //   store object at (start_offset + member n-1 offset)
    // }
    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Struct* s) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, s}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* object = b.FunctionParam("object", s);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, object});

            b.Append(fn->Block(), [&] {
                for (auto* member : s->Members()) {
                    uint32_t mem_offset = static_cast<uint32_t>(member->Offset());
                    OffsetData offset{mem_offset, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(offset);
                    auto* from = b.Access(member->Type(), object, u32(member->Index()))->Result();
                    MakeStore(inst, var, from, byte_idx);
                }
                b.Return(fn);
            });

            return fn;
        });
    }

    // Create a store function for the given `var` and `matrix` combination. Essentially creates a
    // function similar to:
    //
    // fn custom_store_M(start_offset:u32, object:M) {
    //   store object[0] at offset (start_offset + 0)
    //   store object[1] at offset (start_offset + (1 * ColumnStride))
    //   store object[2] at offset (start_offset + (2 * ColumnStride))
    //   store object[3] at offset (start_offset + (3 * ColumnStride))
    // }
    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Matrix* m) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, m}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* object = b.FunctionParam("object", m);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, object});

            b.Append(fn->Block(), [&] {
                for (uint32_t c = 0; c < m->Columns(); c++) {
                    uint32_t vec_offset = static_cast<uint32_t>(c * m->ColumnStride());
                    OffsetData offset{vec_offset, {start_byte_offset}};
                    auto* byte_idx = OffsetToValue(offset);
                    auto* from = b.Access(m->ColumnType(), object, u32(c))->Result();
                    MakeStore(inst, var, from, byte_idx);
                }
                b.Return(fn);
            });

            return fn;
        });
    }

    // Create a store function for the given `var` and `array` combination. Essentially creates a
    // function similar to:
    //
    // fn custom_store_A(start_offset:u32, object:A) {
    // }
    core::ir::Function* GetStoreFunctionFor(core::ir::Instruction* inst,
                                            core::ir::Var* var,
                                            const core::type::Array* a) {
        return var_and_type_to_store_fn_.GetOrAdd(VarTypePair{var, a}, [&] {
            auto* start_byte_offset = b.FunctionParam("start_byte_offset", ty.u32());
            auto* object = b.FunctionParam("object", a);
            auto* fn = b.Function(ty.void_());
            fn->SetParams({start_byte_offset, object});

            b.Append(fn->Block(), [&] {
                auto* count = a->Count()->As<core::type::ConstantArrayCount>();
                TINT_IR_ASSERT(ir, count);

                b.LoopRange(0_u, u32(count->value), 1_u, [&](core::ir::Value* idx) {
                    auto* stride = b.Multiply(idx, u32(a->ImplicitStride()))->Result();
                    OffsetData od{0, {start_byte_offset, stride}};
                    auto* byte_idx = OffsetToValue(od);
                    auto* from = b.Access(a->ElemType(), object, idx)->Result();
                    MakeStore(inst, var, from, byte_idx);
                });

                b.Return(fn);
            });

            return fn;
        });
    }
};

}  // namespace

Result<SuccessType> DecomposeAccess(core::ir::Module& ir, const DecomposeAccessOptions& options) {
    TINT_CHECK_RESULT(
        ValidateAndDumpIfNeeded(ir, "core.DecomposeUniformAccess",
                                core::ir::Capabilities{
                                    core::ir::Capability::kAllow8BitIntegers,
                                    core::ir::Capability::kAllow16BitIntegers,
                                    core::ir::Capability::kAllowClipDistancesOnF32ScalarAndVector,
                                    core::ir::Capability::kAllowDuplicateBindings,
                                    core::ir::Capability::kAllowNonCoreTypes,
                                }));

    State{ir, options}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
