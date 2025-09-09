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

#include "dawn/replay/Replay.h"

#include <type_traits>

#include "dawn/replay/Serialization.h"

namespace dawn::replay {

Replay* Replay::Create(WGPUDevice device, const Capture* capture) {
  return new Replay(device, capture);  
}

Replay::Replay(WGPUDevice device, const Capture* capture) : mDevice(device), mCapture(capture) {
  
}

template <typename T>
bool Read(ReadHead& readHead, T* value,
          typename std::enable_if<
              std::is_same<T, uint32_t>::value ||
              (std::is_enum<T>::value && std::is_same<typename std::underlying_type<T>::type, uint32_t>::value),
              void
          >::type* = nullptr) {
    readHead.read(reinterpret_cast<char*>(value), sizeof(T));
    return !!readHead;
}

bool Read(ReadHead& readHead, uint64_t* value) {
    readHead.read(reinterpret_cast<char*>(value), sizeof(value));
    return !!readHead;
}

bool CreateBuffer(WGPUDevice device, ReadHead& readHead) {
  return true;
}

bool CreateResource(WGPUDevice device, ReadHead& readHead) {
    ObjectType objectType;
    ObjectId objectId;
    Read(readHead, &objectType);
    Read(readHead, &objectId);

    switch (objectType) {
        case ObjectType::Buffer: {
            CreateBuffer(device, readHead);
            break;
        }

        default:
            // UNIMPLEMENTED();
          break;
    }
    return true;
}

void Replay::Play() {
    auto readHead = mCapture->GetReadHead();
    RootCommand cmd;
    
    while (Read(readHead, &cmd)) {
      switch (cmd) {
        case RootCommand::CreateResource: {
          CreateResource(mDevice, readHead);
          break;
        }
        case RootCommand::WriteBuffer: {
          break;
        }
        default: {
          // UNIMPLEMENTED();
          break;
        }
      }
    }
}


}  // namespace dawn::replay