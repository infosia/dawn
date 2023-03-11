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

#ifndef SRC_TINT_INTERP_WORKGROUP_H_
#define SRC_TINT_INTERP_WORKGROUP_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "tint/interp/uvec3.h"

namespace tint::interp {
class Invocation;
class Memory;
class ShaderExecutor;
}  // namespace tint::interp

namespace tint::interp {

/// A Workgroup object represents a single workgroup in a shader dispatch, and contains a set of
/// invocations.
class Workgroup {
  public:
    /// Constructor.
    /// @param executor the ShaderExecutor
    /// @param group_id the workgroup ID
    /// @param size the workgroup size
    Workgroup(ShaderExecutor& executor, UVec3 group_id, UVec3 size);

    /// @returns the workgroup ID of this workgroup
    const UVec3& GroupId() const { return group_id_; }

    /// @returns the size of this workgroup
    const UVec3& Size() const { return size_; }

    /// @returns the invocation that is currently running
    Invocation* CurrentInvocation() const { return current_invocation_; }

    /// Switch the invocation that is currently executing.
    /// @param local_id the local invocation ID of the invocation to switch to
    /// @returns true on success, false if the ID is invalid or the invocation has already finished
    bool SelectInvocation(UVec3 local_id);

    /// Step the workgroup.
    void Step();

    /// @returns true if the workgroup has completed execution
    bool IsFinished() const {
        return current_invocation_ == nullptr && ready_.empty() && barrier_.empty();
    }

  private:
    ShaderExecutor& executor_;
    UVec3 group_id_;
    UVec3 size_;
    std::vector<std::unique_ptr<Memory>> allocations_;

    std::vector<std::unique_ptr<Invocation>> invocations_;
    Invocation* current_invocation_ = nullptr;
    std::map<UVec3, Invocation*> ready_;
    std::vector<Invocation*> barrier_;
};

}  // namespace tint::interp

#endif  // SRC_TINT_INTERP_WORKGROUP_H_
