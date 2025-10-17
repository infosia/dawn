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

#include "src/tint/lang/hlsl/writer/raise/array_offset_from_uniform.h"

#include <algorithm>
#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/hlsl/ir/member_builtin_call.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

// Forward declarations.
namespace tint::core::ir {
class Module;
}  // namespace tint::core::ir

namespace tint::hlsl::writer::raise {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The binding point to use for the uniform buffer.
    BindingPoint ubo_binding;

    /// The map from binding point to the element index which holds the offset into that buffer.
    const std::unordered_map<BindingPoint, uint32_t>& bindpoint_to_offset_index;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// Process the module.
    void Process() {
        // TODO: find the vars with bindpoint in bindpoint_to_offset_index
        //     for each var
        //         for each usage
        //             if load/store (access?), replace the offset with offset + bindpoint_to_offset_index.offset

        for (auto* inst : *ir.root_block) {
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }

            if (var->BindingPoint()) {
                auto it = bindpoint_to_offset_index.find(*var->BindingPoint());
                if (it != bindpoint_to_offset_index.end()) {
                    // Copy because we destroy elements while iterating
                    auto usages_unsorted_copy = var->Result()->UsagesUnsorted();
                    for (auto usage : usages_unsorted_copy) {
                        tint::Switch(usage->instruction,
                            // [&](core::ir::Access* access) {
                            //     b.InsertBefore(access, [&] {
                            //         auto* dyn_offset = b.Let("tint_dynamic_offset", u32(it->second));
                            //         auto* new_offset = b.Add(ty.u32(), access->Indices()[0], dyn_offset);
                            //         auto indices = tint::ToVector<4>(access->Indices());
                            //         indices[0] = new_offset->Result();
                            //         auto* new_access = b.Access(access->Result()->Type(), access->Object(), std::move(indices));
                            //         access->Result()->ReplaceAllUsesWith(new_access->Result());
                            //     });
                            //     access->Destroy();
                            // },
                            [&](hlsl::ir::MemberBuiltinCall* mbc) {
                                if (!IsLoad(mbc->Func()) && !IsStore(mbc->Func())) {
                                    return;
                                }

                                b.InsertBefore(mbc, [&] {
                                    // Load the storage buffer dynamic offset from the uniform buffer.
                                    // The offsets are packed into vec4s to satisfy the 16-byte alignment requireement for array elements in uniform buffers, so we have to find the vector and element that corresponds to the index we want.
                                    core::ir::Value* dyn_offset = nullptr;
                                    {
                                        const uint32_t offset_index = it->second;
                                        const uint32_t array_index = offset_index / 4;
                                        const uint32_t vec_index = offset_index % 4;
                                        auto* vec_ptr = b.Access<ptr<uniform, vec4<u32>>>(BufferOffsets(), u32(array_index));
                                        dyn_offset = b.LoadVectorElement(vec_ptr, u32(vec_index))->Result();
                                    }                                    

                                    // For both byte_address_buffer load and store member functions, the offset is the first parameter.
                                    core::ir::Value* curr_offset = mbc->Args()[0];
                                    // auto* dyn_offset = b.Let("tint_dynamic_offset", u32(it->second));
                                    auto* new_offset = b.Add(ty.u32(), curr_offset, dyn_offset);
                                    auto args = tint::ToVector<4>(mbc->Args());
                                    args[0] = new_offset->Result();
                                    auto* new_mbc = b.MemberCall<hlsl::ir::MemberBuiltinCall>(mbc->Result()->Type(), mbc->Func(), mbc->Object(), std::move(args));
                                    mbc->Result()->ReplaceAllUsesWith(new_mbc->Result());
                                });
                                mbc->Destroy();
                            },
                            TINT_ICE_ON_NO_MATCH);
                    }
                }
            }
        }
    }

    /// The uniform buffer variable that holds the dynamic offset of each storage buffer.
    core::ir::Var* buffer_sizes_var = nullptr;

    /// Get (or create, on first call) the uniform buffer that contains the storage buffer offsets.
    /// @returns the uniform buffer pointer
    core::ir::Value* BufferOffsets() {
        if (buffer_sizes_var) {
            return buffer_sizes_var->Result();
        }

        // Find the largest index declared in the map, in order to determine the number of elements
        // needed in the array of buffer sizes.
        // The buffer sizes will be packed into vec4s to satisfy the 16-byte alignment requirement
        // for array elements in uniform buffers.
        uint32_t max_index = 0;
        for (auto& entry : bindpoint_to_offset_index) {
            max_index = std::max(max_index, entry.second);
        }
        uint32_t num_elements = (max_index / 4) + 1;
        b.Append(ir.root_block, [&] {
            buffer_sizes_var = b.Var("tint_storage_buffer_dynamic_offsets",
                                     ty.ptr<uniform>(ty.array(ty.vec4<u32>(), num_elements)));
        });
        buffer_sizes_var->SetBindingPoint(ubo_binding.group, ubo_binding.binding);
        return buffer_sizes_var->Result();
    }

    // core::ir::Value* GetDynamicOffset(
};

}  // namespace

Result<ArrayOffsetFromUniformResult> ArrayOffsetFromUniform(
    core::ir::Module& ir,
    BindingPoint ubo_binding,
    const std::unordered_map<BindingPoint, uint32_t>& bindpoint_to_offset_index) {
    auto validated = ValidateAndDumpIfNeeded(ir, "hlsl.ArrayOffsetFromUniform",
                                             kArrayOffsetFromUniformCapabilities);
    if (validated != Success) {
        return validated.Failure();
    }

    State state{ir, ubo_binding, bindpoint_to_offset_index};
    state.Process();

    ArrayOffsetFromUniformResult result;
    // result.needs_storage_buffer_sizes = state.NeedsStorageBufferSizes();
    return result;
}

}  // namespace tint::hlsl::writer::raise
