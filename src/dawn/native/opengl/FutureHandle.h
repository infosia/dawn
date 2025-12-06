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

#ifndef SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_
#define SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_

#include <atomic>
#include <type_traits>
#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/NonCopyable.h"
#include "dawn/common/RefCounted.h"
#include "dawn/native/opengl/opengl_platform.h"

namespace dawn::native::opengl {

// Wrapper of a handle that allows the handle to be created asynchronously.
// When the handle is queried, an assert will ensure that the handle is already created.
template <typename T>
class FutureHandleBase : public RefCounted, public NonCopyable {
  public:
    FutureHandleBase() = default;
    explicit FutureHandleBase(T value) : mValue(std::move(value)), mIsSet(true) {}

    void Set(T value) {
        mValue = std::move(value);
        DAWN_ASSERT(!mIsSet.exchange(true, std::memory_order_release));
    }

    using ReturnType =
        std::conditional_t<std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,
                           T,
                           const T&>;

    ReturnType Get() const {
        // Assert that the handle is already initialized.
        DAWN_ASSERT(mIsSet.load(std::memory_order_acquire));
        return mValue;
    }

    // Unlike Get(), this is safe to be called even if the value is not set.
    // If it's not set, a default value will be returned.
    ReturnType GetOrDefault() const {
        DAWN_ASSERT(mIsSet.load(std::memory_order_acquire) || mValue == T{});
        return mValue;
    }

    T Detach() {
        DAWN_ASSERT(mIsSet.load(std::memory_order_acquire));
        return std::move(mValue);
    }

  private:
    T mValue{};
    std::atomic<bool> mIsSet = false;
};

using FutureHandle = FutureHandleBase<GLuint>;

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_
