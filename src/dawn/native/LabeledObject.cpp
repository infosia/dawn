// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/native/LabeledObject.h"

#include <string>
#include <utility>

namespace dawn::native {

LabeledObject::~LabeledObject() {
    delete mLabel.load(std::memory_order_acquire);
}

void LabeledObject::SetLabel(std::string label) {
    auto* labelObj = mLabel.load(std::memory_order_acquire);
    if (labelObj == nullptr) {
        auto* newLabel = new MutexProtected<std::string>();
        MutexProtected<std::string>* expected = nullptr;
        if (mLabel.compare_exchange_strong(expected, newLabel, std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
            labelObj = newLabel;
        } else {
            delete newLabel;
            labelObj = expected;
        }
    }
    labelObj->Use([&label](auto guard) { *guard = std::move(label); });
}

std::string LabeledObject::GetLabel() const {
    auto* label = mLabel.load(std::memory_order_acquire);
    if (label == nullptr) {
        return "";
    }
    return label->Use([](auto guard) { return std::string(*guard); });
}

}  // namespace dawn::native
