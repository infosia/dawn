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

#include "dawn/replay/Capture.h"

#include <span>
#include <sstream>

namespace dawn::replay {

Capture* Capture::Create(CaptureStream& stream, size_t size) {
    std::vector<uint8_t> data;
    data.resize(size);
    stream.read(reinterpret_cast<char*>(data.data()), size);
    return new Capture(std::move(data));
}

Capture::Capture(std::vector<uint8_t> data) : mData(std::move(data)) {
}

Capture::~Capture() {
}

class SpanStreamBuf : public std::streambuf {
public:
    SpanStreamBuf(std::span<const char> span) {
        char* begin = const_cast<char*>(span.data());
        this->setg(begin, begin, begin + span.size());
    }
};

ReadHead Capture::GetReadHead() const {
    std::span<const uint8_t> data_span = mData;
    std::span<const char> char_span(
        reinterpret_cast<const char*>(data_span.data()),
        data_span.size()
    );
    SpanStreamBuf buf(char_span);
    return ReadHead(&buf);
}

}  // namespace dawn::replay
