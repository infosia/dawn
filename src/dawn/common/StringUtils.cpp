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

#include "dawn/common/StringUtils.h"

#include "dawn/webgpu.h"

namespace dawn {

std::string_view FromWGPUString(const WGPUString& string) {
    const bool isNull = string.data == nullptr;
    const bool useStrlen = string.length == SIZE_MAX;
    return std::string_view(string.data, isNull      ? 0
                                         : useStrlen ? strlen(string.data)
                                                     : string.length);
}

std::optional<std::string_view> FromOptionalWGPUString(const WGPUString& string) {
    const bool isNull = string.data == nullptr;
    const bool useStrlen = string.length == SIZE_MAX;
    if (isNull && useStrlen) {
        return std::nullopt;
    }
    return std::string_view(string.data, isNull      ? 0
                                         : useStrlen ? strlen(string.data)
                                                     : string.length);
}

void CopyWGPUString(const WGPUString& in, WGPUString* out, std::string* storage) {
    *out = in;
    if (in.data != nullptr) {
        *storage = FromWGPUString(in);
        out->data = storage->data();
    }
}

}  // namespace dawn
