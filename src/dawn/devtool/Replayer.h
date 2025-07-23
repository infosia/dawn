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

#ifndef SRC_DAWN_DEVTOOL_REPLAYER_H_
#define SRC_DAWN_DEVTOOL_REPLAYER_H_

#include <webgpu/webgpu.h>

#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "dawn/common/Assert.h"
#include "dawn/devtool/CmdPb.h"
#include "dawn/devtool/ObjectType.h"

namespace dawn::devtool {

using ObjectId = uint64_t;

class Replayer {
  public:
    void StartPlayback(WGPUDevice device, std::istream& in);
    void End();

    // Get a pointer to the WGPU* object given an ObjectId.
    template <typename WGPUHandle>
    bool GetFromId(ObjectId id, WGPUHandle* out) const {
        const auto& map = mObjectIdMaps[ObjectTypeOf<WGPUHandle>];
        auto result = map.find(id);
        if (result == map.end()) {
            return false;
        }
        *out = reinterpret_cast<WGPUHandle>(result->second);
        return true;
    }

    // Add an ObjectId and WGPU* object pair to the indexing.
    // Called during playback for:
    //   - Command that creates a new object.
    //   - Objects exist outside of the record range (e.g. Device, default Queue).
    template <typename WGPUHandle>
    bool SetId(WGPUHandle object, ObjectId id) {
        DAWN_ASSERT(!mObjectIdMaps[ObjectTypeOf<WGPUHandle>].contains(id));
        mObjectIdMaps[ObjectTypeOf<WGPUHandle>].emplace(id, object);
        return true;
    }

    // TODO(crbug.com/413053623): More playback control API

  private:
    // Playback
    void InitFromSnapshotGlobalState(WGPUDevice device,
                                     const cmd::pb::SnapshotGlobalState& globalState);

    // ObjectId to WGPU* object map per object type.
    PerObjectType<absl::flat_hash_map<ObjectId, void*>> mObjectIdMaps;

    cmd::pb::Block mBlock;
};

}  // namespace dawn::devtool

#endif  // SRC_DAWN_DEVTOOL_REPLAYER_H_
