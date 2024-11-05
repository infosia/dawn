// Copyright 2022 The Dawn & Tint Authors
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

#include "dawn/native/opengl/UtilsEGL.h"

#include <string>

#include "dawn/native/opengl/EGLFunctions.h"

namespace dawn::native::opengl {

const char* EGLErrorAsString(EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "<Unknown EGL error>";
    }
}

MaybeError CheckEGL(const EGLFunctions& egl, EGLBoolean result, const char* context) {
    if (DAWN_LIKELY(result != EGL_FALSE)) {
        return {};
    }
    EGLint error = egl.GetError();
    std::string message = std::string(context) + " failed with " + EGLErrorAsString(error);
    if (error == EGL_BAD_ALLOC) {
        return DAWN_OUT_OF_MEMORY_ERROR(message);
    } else if (error == EGL_CONTEXT_LOST) {
        return DAWN_DEVICE_LOST_ERROR(message);
    } else {
        return DAWN_INTERNAL_ERROR(message);
    }
}
ResultOrError<Ref<WrappedEGLSync>> WrappedEGLSync::Create(Device* device,
                                                          EGLenum type,
                                                          const EGLAttrib* attribs) {
    const EGLFunctions& egl = device->GetEGL(true);
    EGLSyncKHR sync = egl.CreateSync(device->GetEGLDisplay(), type, attribs);
    DAWN_TRY(CheckEGL(egl, sync != EGL_NO_SYNC, "eglCreateSync"));
    return AcquireRef(new WrappedEGLSync(device, sync));
}

WrappedEGLSync::WrappedEGLSync() {}

WrappedEGLSync::WrappedEGLSync(Device* device, EGLSync sync) : mDevice(device), mSync(sync) {
    DAWN_ASSERT(mDevice != nullptr);
    DAWN_ASSERT(mSync != EGL_NO_SYNC);
}

WrappedEGLSync::~WrappedEGLSync() {
    if (IsValid()) {
        DAWN_ASSERT(mDevice != nullptr);
        const EGLFunctions& egl = mDevice->GetEGL(false);
        egl.DestroySync(mDevice->GetEGLDisplay(), mSync);
        mDevice = nullptr;
        mSync = EGL_NO_SYNC;
    }
}

bool WrappedEGLSync::IsValid() const {
    return mSync != EGL_NO_SYNC;
}

EGLSync WrappedEGLSync::Get() const {
    return mSync;
}

MaybeError WrappedEGLSync::Signal(EGLenum mode) {
    DAWN_ASSERT(IsValid());

    const EGLFunctions& egl = mDevice->GetEGL(false);
    DAWN_TRY(CheckEGL(egl, egl.SignalSync(mDevice->GetEGLDisplay(), mSync, mode), "eglSignalSync"));
    return {};
}

ResultOrError<EGLenum> WrappedEGLSync::ClientWait(EGLint flags, Nanoseconds timeout) {
    DAWN_ASSERT(IsValid());

    const EGLFunctions& egl = mDevice->GetEGL(false);
    EGLenum result = egl.ClientWaitSync(mDevice->GetEGLDisplay(), mSync, flags, uint64_t(timeout));
    DAWN_TRY(CheckEGL(egl, result != EGL_FALSE, "eglClientWaitSync"));
    return result;
}

MaybeError WrappedEGLSync::Wait() {
    DAWN_ASSERT(IsValid());

    const EGLFunctions& egl = mDevice->GetEGL(false);
    constexpr EGLint flags = 0;
    DAWN_TRY(CheckEGL(egl, egl.WaitSync(mDevice->GetEGLDisplay(), mSync, flags), "eglWaitSync"));
    return {};
}

ResultOrError<EGLint> WrappedEGLSync::DupFD() {
    DAWN_ASSERT(IsValid());

    const EGLFunctions& egl = mDevice->GetEGL(false);
    EGLint fd = egl.DupNativeFenceFD(mDevice->GetEGLDisplay(), mSync);
    DAWN_TRY(CheckEGL(egl, fd != EGL_NO_NATIVE_FENCE_FD_ANDROID, "eglDupNativeFenceFDANDROID"));

    return fd;
}

}  // namespace dawn::native::opengl
