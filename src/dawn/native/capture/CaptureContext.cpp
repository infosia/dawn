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

#include "src/dawn/native/capture/CaptureContext.h"

#include "src/dawn/native/capture/Serialization.h"

namespace dawn::native {

CaptureContext::CaptureContext(std::ostream& outputStream)
    : mOutputStream(outputStream) {
    mOutputStream.write(reinterpret_cast<const char*>(capture::kMagicNumber), sizeof(capture::kMagicNumber));
    WriteU32(capture::kVersionNumber);
}

CaptureContext::~CaptureContext() = default;

void CaptureContext::WriteF32(float value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteI32(int32_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteU32(uint32_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteU64(uint64_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteString(const std::string& value) {
    WriteU64(value.length());
    mOutputStream.write(value.c_str(), value.length());
}

void CaptureContext::WriteData(const void* data, size_t size) {
    WriteU64(size);
    mOutputStream.write(reinterpret_cast<const char*>(data), size);
}

}  // namespace dawn::native
