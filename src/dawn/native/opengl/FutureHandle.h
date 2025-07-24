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

#ifndef SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_
#define SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_

#include <future>
#include <type_traits>

#include "dawn/common/Assert.h"
#include "dawn/native/opengl/opengl_platform.h"

namespace dawn::native::opengl {

template <typename T>
class FutureHandleBase {
  public:
    FutureHandleBase() = default;
    explicit FutureHandleBase(std::shared_future<T> future)
        : mHandleFuture(std::move(future)) {}

    using ReturnType =
        std::conditional_t<std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,
                           T,
                           const T&>;

    operator ReturnType() const {
        DAWN_ASSERT(mHandleFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        return mHandleFuture.get();
    }

    ReturnType Get() const { return mHandleFuture.get(); }

    T Detach() { return std::move(mHandleFuture).get(); }

  private:
    std::shared_future<T> mHandleFuture;
};

using FutureHandle = FutureHandleBase<GLuint>;

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_FUTUREHANDLE_H_