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

#ifndef SRC_DAWN_NATIVE_CAPTURE_CAPTURECONTEXT_H_
#define SRC_DAWN_NATIVE_CAPTURE_CAPTURECONTEXT_H_

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "dawn/native/ObjectBase.h"

namespace dawn::native {

class CaptureContext;
class CommandBufferBase;

using ObjectId = uint64_t;

template <typename T>
void CaptureContent(CaptureContext& captureContext, const T* object);

class CaptureContext {
  public:
    explicit CaptureContext(std::ostream& outputStream);
    ~CaptureContext();

    template <typename T>
    bool AddResource(const T* object) {
        assert(object != nullptr);

        auto it = mObjectIds.find(static_cast<const void*>(object));
        if (it != mObjectIds.end()) {
            return false;  // Object was already captured
        }

        CaptureContent(*this, object);

        ObjectId newId = mNextObjectId++;
        mObjectIds[object] = newId;

        return true;
    }

    template <typename T>
    ObjectId GetId(const T* object) {
        if (object == nullptr) {
            return 0;
        }

        auto it = mObjectIds.find(static_cast<const void*>(object));
        assert(it != mObjectIds.end());
        return it->second;
    }

    void WriteBytes(const void* data, size_t size);
    void CaptureCommandBuffer(uint32_t commandCount, CommandBufferBase* const* commands);

  private:
    std::ostream& mOutputStream;
    std::unordered_map<const void*, ObjectId> mObjectIds;
    ObjectId mNextObjectId = 1;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_CAPTURE_CAPTURECONTEXT_H_
