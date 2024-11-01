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

#include "src/tint/lang/core/ir/transform/print.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/result/result.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    Module& ir;

    /// The binding point to use for the storage buffer.
    BindingPoint buffer_binding;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The PrintEntry struct type.
    core::type::Struct* print_entry_struct = nullptr;

    /// The storage buffer variable for the print buffer.
    Var* print_buffer = nullptr;

    /// The private variable that holds the invocation ID.
    Var* invocation_id = nullptr;

    /// The result of the transform.
    PrintResult result{};

    /// Process the module.
    PrintResult Process() {
        // Look for and replace calls to the print builtin.
        for (auto* inst : ir.Instructions()) {
            if (auto* call = inst->As<CoreBuiltinCall>()) {
                if (call->Func() == BuiltinFn::kPrint) {
                    Replace(call);
                }
            }
        }

        return std::move(result);
    }

    /// Replace a call to a print builtin.
    /// @param call the print call to replace
    void Replace(CoreBuiltinCall* call) {
        SetupGlobals();

        b.InsertBefore(call, [&] {
            // Generate an ID for the print call. Zero is reserved for "invalid entry".
            auto id = result.print_id_to_info.size() + 1;
            auto* context = b.Construct<vec4<u32>>(u32(id), b.Load(invocation_id));
            auto* arg = call->Args()[0];
            auto* data = ConvertValue(arg);
            auto* entry = b.Construct(print_entry_struct, context, data);

            // Atomically increment the print entry index.
            auto* index_ptr =
                b.Access(ty.ptr(AddressSpace::kStorage, ty.atomic<u32>(), core::Access::kReadWrite),
                         print_buffer, 0_u);
            auto* index = b.Call<u32>(BuiltinFn::kAtomicAdd, index_ptr, 1_u);

            // Write the entry to the print buffer the index is in bounds.
            auto* bounds_check = b.If(b.LessThan<bool>(index, u32(kMaxPrintEntries)));
            b.Append(bounds_check->True(), [&] {
                auto* entry_ptr = b.Access(
                    ty.ptr(AddressSpace::kStorage, print_entry_struct, core::Access::kReadWrite),
                    print_buffer, 1_u, index);
                b.Store(entry_ptr, entry);
                b.ExitIf(bounds_check);
            });

            // Record information about the print call.
            auto location = ir.SourceOf(call).range.begin;
            auto info = PrintInfo{location, TypeToPrintValueType(arg->Type())};
            result.print_id_to_info.emplace(id, std::move(info));
        });

        call->Destroy();
    }

    /// Convert a value to a vec4u.
    /// @param value the value to convert
    /// @returns the converted value
    ir::Value* ConvertValue(ir::Value* value) {
        auto bitcast_u32 = [&](auto* src) {
            return b.Bitcast(ty.MatchWidth(ty.u32(), value->Type()), src)->Result(0);
        };
        auto convert = [&](const type::Scalar* el_ty) {
            return b.Convert(ty.MatchWidth(el_ty, value->Type()), value)->Result(0);
        };
        return tint::Switch(
            value->Type(),
            [&](const type::Scalar* scalar) {
                auto* word = tint::Switch(
                    scalar,                                                            //
                    [&](const type::Bool*) { return convert(ty.u32()); },              //
                    [&](const type::F16*) { return bitcast_u32(convert(ty.f32())); },  //
                    [&](const type::F32*) { return bitcast_u32(value); },              //
                    [&](const type::I32*) { return bitcast_u32(value); },              //
                    [&](const type::U32*) { return value; },                           //
                    TINT_ICE_ON_NO_MATCH);
                return b.Construct<vec4<u32>>(word, 0_u, 0_u, 0_u)->Result(0);
            },
            [&](const type::Vector* vector) -> ir::Value* {
                ir::Value* converted = tint::Switch(
                    vector->Type(),  //
                    [&](const type::Bool*) -> ir::Value* { return convert(ty.u32()); },
                    [&](const type::F16*) -> ir::Value* { return bitcast_u32(convert(ty.f32())); },
                    [&](const type::F32*) -> ir::Value* { return bitcast_u32(value); },
                    [&](const type::I32*) -> ir::Value* { return bitcast_u32(value); },
                    [&](const type::U32*) -> ir::Value* { return value; }, TINT_ICE_ON_NO_MATCH);

                if (vector->Width() == 2) {
                    return b.Construct<vec4<u32>>(converted, 0_u, 0_u)->Result(0);
                } else if (vector->Width() == 3) {
                    return b.Construct<vec4<u32>>(converted, 0_u)->Result(0);
                }
                return converted;
            },
            TINT_ICE_ON_NO_MATCH);
    }

    /// Convert a type to the corresponding PrintValueType enum value.
    /// @param type the type
    /// @returns the enum value
    PrintValueType TypeToPrintValueType(const type::Type* type) {
        return tint::Switch(
            type,  //
            [&](const type::Bool*) { return PrintValueType::kBool; },
            [&](const type::F16*) { return PrintValueType::kF16; },
            [&](const type::F32*) { return PrintValueType::kF32; },
            [&](const type::I32*) { return PrintValueType::kI32; },
            [&](const type::U32*) { return PrintValueType::kU32; },
            [&](const type::Vector* vector) {
                return tint::Switch(
                    vector->Type(),
                    [&](const type::Bool*) {
                        switch (vector->Width()) {
                            case 2:
                                return PrintValueType::kVec2Bool;
                            case 3:
                                return PrintValueType::kVec3Bool;
                            case 4:
                                return PrintValueType::kVec4Bool;
                        }
                        TINT_UNREACHABLE();
                    },
                    [&](const type::F16*) {
                        switch (vector->Width()) {
                            case 2:
                                return PrintValueType::kVec2F16;
                            case 3:
                                return PrintValueType::kVec3F16;
                            case 4:
                                return PrintValueType::kVec4F16;
                        }
                        TINT_UNREACHABLE();
                    },
                    [&](const type::F32*) {
                        switch (vector->Width()) {
                            case 2:
                                return PrintValueType::kVec2F32;
                            case 3:
                                return PrintValueType::kVec3F32;
                            case 4:
                                return PrintValueType::kVec4F32;
                        }
                        TINT_UNREACHABLE();
                    },
                    [&](const type::I32*) {
                        switch (vector->Width()) {
                            case 2:
                                return PrintValueType::kVec2I32;
                            case 3:
                                return PrintValueType::kVec3I32;
                            case 4:
                                return PrintValueType::kVec4I32;
                        }
                        TINT_UNREACHABLE();
                    },
                    [&](const type::U32*) {
                        switch (vector->Width()) {
                            case 2:
                                return PrintValueType::kVec2U32;
                            case 3:
                                return PrintValueType::kVec3U32;
                            case 4:
                                return PrintValueType::kVec4U32;
                        }
                        TINT_UNREACHABLE();
                    },
                    TINT_ICE_ON_NO_MATCH);
            },
            TINT_ICE_ON_NO_MATCH);
    }

    /// Set up the global structures and variables.
    void SetupGlobals() {
        if (invocation_id) {
            return;
        }

        // Create the invocation ID variable.
        invocation_id =
            b.Var("tint_print_invocation_id", core::AddressSpace::kPrivate, ty.vec3<u32>());
        ir.root_block->Append(invocation_id);
        for (auto func : ir.functions) {
            switch (func->Stage()) {
                case Function::PipelineStage::kCompute:
                    SetupComputeInvocationId(func);
                    break;
                case Function::PipelineStage::kFragment:
                    SetupFragmentInvocationId(func);
                    break;
                case Function::PipelineStage::kVertex:
                    SetupVertexInvocationId(func);
                    break;
                case Function::PipelineStage::kUndefined:
                    break;
            }
        }

        // Create the print buffer struct and variable.
        auto* print_entry_type = PrintEntryStruct();
        auto* print_buffer_type = ty.Struct(ir.symbols.New("TintPrintBuffer"),
                                            {
                                                {
                                                    ir.symbols.Register("print_index"),
                                                    ty.atomic<u32>(),
                                                },
                                                {
                                                    ir.symbols.Register("print_entries"),
                                                    ty.array(print_entry_type, kMaxPrintEntries),
                                                },
                                            });
        TINT_ASSERT(print_buffer_type->Members()[1]->Offset() == kPrintEntryOffset);
        print_buffer = b.Var("tint_print_buffer", core::AddressSpace::kStorage, print_buffer_type,
                             core::Access::kReadWrite);
        print_buffer->SetBindingPoint(buffer_binding.group, buffer_binding.binding);
        ir.root_block->Append(print_buffer);
    }

    /// Get (or create) the print entry struct type.
    /// @returns the struct type
    core::type::Struct* PrintEntryStruct() {
        if (!print_entry_struct) {
            print_entry_struct = ty.Struct(ir.symbols.New("TintPrintEntry"),
                                           {
                                               {ir.symbols.Register("context"), ty.vec4<u32>()},
                                               {ir.symbols.Register("data"), ty.vec4<u32>()},
                                           });
            TINT_ASSERT(print_entry_struct->Size() == kPrintEntrySize);
        }
        return print_entry_struct;
    }

    /// Set up the invocation ID for a compute shader.
    /// @param ep the compute shader entry point
    void SetupComputeInvocationId(ir::Function* ep) {
        auto* id = GetBuiltinInput(ep, BuiltinValue::kGlobalInvocationId, ty.vec3<u32>());
        b.InsertBefore(ep->Block()->Front(), [&] {  //
            b.Store(invocation_id, id);
        });
    }

    /// Set up the invocation ID for a fragment shader.
    /// @param ep the fragment shader entry point
    void SetupFragmentInvocationId(ir::Function* ep) {
        auto* pos = GetBuiltinInput(ep, BuiltinValue::kPosition, ty.vec4<f32>());
        b.InsertBefore(ep->Block()->Front(), [&] {  //
            b.Store(invocation_id,
                    b.Bitcast<vec3<u32>>(b.Swizzle<vec3<f32>>(pos, Vector{0u, 1u, 2u})));
        });
    }

    /// Set up the invocation ID for a vertex shader.
    /// @param ep the vertex shader entry point
    void SetupVertexInvocationId(ir::Function* ep) {
        auto* instance = GetBuiltinInput(ep, BuiltinValue::kInstanceIndex, ty.u32());
        auto* vertex = GetBuiltinInput(ep, BuiltinValue::kVertexIndex, ty.u32());
        b.InsertBefore(ep->Block()->Front(), [&] {  //
            b.Store(invocation_id, b.Construct<vec3<u32>>(instance, vertex, 0_u));
        });
    }

    /// Get or insert a shader builtin input value for an entry point.
    /// @param ep the entry point
    /// @param builtin the builtin attribute
    /// @param type the type of the builtin
    /// @returns the shader builtin input value
    ir::Value* GetBuiltinInput(ir::Function* ep, BuiltinValue builtin, const type::Type* type) {
        // Check every shader input to see if the builtin already exists.
        for (auto* param : ep->Params()) {
            if (auto* strct = param->Type()->As<type::Struct>()) {
                for (auto* member : strct->Members()) {
                    if (member->Attributes().builtin == builtin) {
                        return b.Access(type, param, u32(member->Index()))->Result(0);
                    }
                }
            } else {
                if (param->Builtin() == builtin) {
                    return param;
                }
            }
        }

        // We did not find the builtin, so invent one as a new parameter.
        auto* param = b.FunctionParam(ir.symbols.New().Name(), type);
        param->SetBuiltin(builtin);
        ep->AppendParam(param);
        return param;
    }
};

}  // namespace

Result<PrintResult> Print(Module& ir, BindingPoint buffer_binding) {
    auto validated = ValidateAndDumpIfNeeded(ir, "core.Print");
    if (validated != Success) {
        return validated.Failure();
    }

    State state{ir, buffer_binding};
    return state.Process();
}

}  // namespace tint::core::ir::transform
