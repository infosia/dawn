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

#ifndef SRC_DAWN_NATIVE_WEBGPU_WEBGPUERROR_H_
#define SRC_DAWN_NATIVE_WEBGPU_WEBGPUERROR_H_

#include <string>

#include "dawn/native/Error.h"
#include "dawn/native/ErrorInjector.h"

namespace dawn::native::webgpu {

constexpr WGPUWaitStatus WGPU_WAIT_STATUS_FAKE_ERROR_FOR_TESTING = WGPUWaitStatus_Error;

constexpr wgpu::ErrorType WGPU_FAKE_ERROR_FOR_TESTING = wgpu::ErrorType::Unknown;
MaybeError CheckWGPUSuccessImpl(wgpu::ErrorType result, const char* context);

// Returns a success only if result is wgpu::ErrorType::NoError, an error with the context and
// stringified result value instead. Can be used like this:
//
//   DAWN_TRY(CheckWGPUSuccess(wgpuDoSomething, "doing something"));
// Trick: All WebGPU enum type representing success has the same value of 1, so static_cast to
// wgpu::ErroType gets
#define CheckWGPUSuccess(resultIn, contextIn)                                                     \
    ::dawn::native::webgpu::CheckWGPUSuccessImpl(                                                 \
        INJECT_ERROR_OR_RUN(static_cast<wgpu::ErrorType>(resultIn), WGPU_FAKE_ERROR_FOR_TESTING), \
        contextIn)

MaybeError CheckWGPUWaitStatusSuccessImpl(WGPUWaitStatus result, const char* context);

// Returns a success only if result is success, an error with the context and stringified
// result value instead. Can be used like this:
//
//   DAWN_TRY(CheckWGPUWaitStatusSuccess(wgpuDoSomething, "doing something"));
// Trick: All WebGPU enum type representing success has the same value of 1, so static_cast to
// wgpu::ErroType gets
#define CheckWGPUWaitStatusSuccess(resultIn, contextIn)     \
    ::dawn::native::webgpu::CheckWGPUWaitStatusSuccessImpl( \
        INJECT_ERROR_OR_RUN(resultIn, WGPU_WAIT_STATUS_FAKE_ERROR_FOR_TESTING), contextIn)

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_WEBGPUERROR_H_
