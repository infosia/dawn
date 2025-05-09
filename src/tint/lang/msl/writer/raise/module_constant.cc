// Copyright 2023 The Dawn & Tint Authors
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

#include "src/tint/lang/msl/writer/raise/module_constant.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/utils/containers/hashmap.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::msl::writer::raise  {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// Traversal action for WalkAccessChain.
    enum class Action { kStop, kContinue };


    /// Process the module.
    void Process() {
        // Find the access instructions that need replacing.
        // Replace each access instruction that we recorded.
        Hashmap<core::ir::Value*, core::ir::Value*, 16> object_to_var;

        for (auto* inst : ir.Instructions()) {
            if (auto* access = inst->As<core::ir::Access>()) {
                auto* source_object = access->Object();
           
                if (auto curr_const = source_object->As<core::ir::Constant>()) {
                    // only handle arrays for now. Probably also structs
                    if(!curr_const->Type()->Is<core::type::Array>()){
                        continue;
                    }
                    // Declare a variable and copy the source object to it.
                    auto* var = object_to_var.GetOrAdd(source_object, [&] {
                        // If the source object is a constant we use a module-scope variable
                        core::ir::Var* decl = b.Var(ty.ptr(core::AddressSpace::kPrivate, source_object->Type(),
                                                core::Access::kRead));
                        ir.root_block->Append(decl);
                        decl->SetInitializer(source_object);
                        return decl->Result();
                    });
                    access->SetOperand(core::ir::Access::kObjectOperandOffset, var) ;

                    Vector<core::ir::Value*, 4> indices = access->Indices();

               core::ir::Instruction* new_access =
                    b.Access(ty.ptr(core::AddressSpace::kPrivate, access->Result()->Type(), core::Access::kRead), var,indices);
                new_access->InsertBefore(access);
    
                core::ir::Instruction* load =  b.LoadWithResult(access->DetachResult(), new_access);
                
                access->ReplaceWith(load);
                access->Destroy();
                }
            }
        }

    }
};

}  // namespace

Result<SuccessType> ModuleConstant(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "msl.ModuleConstant", kModuleConstant);
    if (result != Success) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::writer::raise
