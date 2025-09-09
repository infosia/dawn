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

#include "dawn/replay/ReadHead.h"

#include <memory>

namespace dawn::replay {

ReadHead::ReadHead(const uint8_t* data, size_t size) : mData(data), mEnd(data + size) {}

bool ReadHead::read(void* data, size_t size) {
    const uint8_t* newData = mData + size;
    if (newData > mEnd) {
        mBad = true;
        return false;
    }
    memcpy(data, mData, size);
    mData = newData;
    return true;
}

const uint8_t* ReadHead::GetData(size_t size) {
    const uint8_t* newData = mData + size;
    if (newData > mEnd) {
        mBad = true;
        return nullptr;
    }
    const uint8_t* data = mData;
    mData = newData;
    return data;
}

bool ReadHead::isBad() const {
    return mBad;
}

bool ReadHead::isDone() const {
    return mBad || mData == mEnd;
}

}  // namespace dawn::replay
