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

#include "dawn/native/interpreter/BufferInterpreter.h"

#include <utility>

#include "dawn/native/ChainUtils.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/interpreter/DeviceInterpreter.h"
#include "tint/interp/memory.h"

namespace dawn::native::interpreter {

// static
ResultOrError<Ref<Buffer>> Buffer::Create(Device* device,
                                          const UnpackedPtr<BufferDescriptor>& descriptor) {
    Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor));
    DAWN_TRY(buffer->Initialize(descriptor->mappedAtCreation));
    return std::move(buffer);
}

Buffer::~Buffer() {}

tint::interp::Memory& Buffer::GetMemory() {
    return *mMemory;
}

MaybeError Buffer::Initialize(bool mappedAtCreation) {
    mAllocatedSize = GetSize();

    // Limit buffer sizes to 16GB.
    if (mAllocatedSize >= (uint64_t(1) << 34)) {
        return DAWN_OUT_OF_MEMORY_ERROR("Buffer size exceeds 16GB");
    }

    mMemory = std::make_unique<tint::interp::Memory>(mAllocatedSize);
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    return {};
}

void Buffer::UnmapImpl() {}

bool Buffer::IsCPUWritableAtCreation() const {
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    return {};
}

void Buffer::FinalizeMapImpl() {}

void* Buffer::GetMappedPointerImpl() {
    return mMemory->Data();
}

}  // namespace dawn::native::interpreter
