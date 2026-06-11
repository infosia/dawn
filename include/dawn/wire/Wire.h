// Copyright 2017 The Dawn & Tint Authors
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

#ifndef INCLUDE_DAWN_WIRE_WIRE_H_
#define INCLUDE_DAWN_WIRE_WIRE_H_

#include <webgpu/webgpu.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>

#include "dawn/wire/dawn_wire_export.h"

namespace dawn::wire {

class DAWN_WIRE_EXPORT CommandSerializer {
  public:
    CommandSerializer();
    virtual ~CommandSerializer();
    CommandSerializer(const CommandSerializer& rhs) = delete;
    CommandSerializer& operator=(const CommandSerializer& rhs) = delete;

    // Get space for serializing commands.
    // GetCmdSpace will never be called with a value larger than
    // what GetMaximumAllocationSize returns. Return nullptr to indicate
    // a fatal error.
    virtual void* GetCmdSpace(size_t size) = 0;
    virtual bool Flush() = 0;
    virtual size_t GetMaximumAllocationSize() const = 0;
    virtual void OnSerializeError();
};

class DAWN_WIRE_EXPORT CommandHandler {
  public:
    CommandHandler();
    virtual ~CommandHandler();
    CommandHandler(const CommandHandler& rhs) = delete;
    CommandHandler& operator=(const CommandHandler& rhs) = delete;

    virtual const volatile char* HandleCommands(const volatile char* commands, size_t size) = 0;
};

// Handle struct that are used to uniquely represent an object of a particular type in the wire.
struct Handle {
    uint32_t id = 0;
    uint32_t generation = 0;

    bool operator==(const Handle& other) const = default;
};

// Identifier serialized between client and server to locate a shared buffer.
struct DAWN_WIRE_EXPORT TransferBufferHandle {
    uint64_t id;
    size_t size;
};

class DAWN_WIRE_EXPORT TransferBufferManager {
  public:
    TransferBufferManager();
    ~TransferBufferManager();

    // Allocates a zero-initialized buffer of the given size and returns its unique ID.
    // The buffer starts with a reference count of 1.
    // Returns 0 on failure.
    uint64_t CreateBuffer(size_t size);

    // Returns true if a buffer with the given ID exists.
    bool HasBuffer(uint64_t id) const;

    // Increments the reference count of the buffer.
    void AddBufferRef(uint64_t id);

    // Decrements the reference count of the buffer. Destroys the buffer when it reaches 0.
    void ReleaseBuffer(uint64_t id);

    // Looks up a previously created buffer by ID. Returns an empty span if not found.
    std::span<const uint8_t> GetBufferPointer(uint64_t id) const;

    // Returns the size of a previously created buffer. Returns 0 if not found.
    size_t GetBufferSize(uint64_t id) const;

  private:
    struct Buffer {
        std::unique_ptr<uint8_t[]> data;
        size_t size = 0;
        uint32_t refCount = 1;
    };

    mutable std::mutex mMutex;
    std::unordered_map<uint64_t, Buffer> mBuffers;
    uint64_t mNextId = 1;
};

}  // namespace dawn::wire

#endif  // INCLUDE_DAWN_WIRE_WIRE_H_
