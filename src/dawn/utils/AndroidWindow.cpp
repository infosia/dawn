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

#include "dawn/utils/AndroidWindow.h"

#include <android/log.h>
#include "dawn/utils/android_native_app_glue.h"

#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Toucan App", __VA_ARGS__))

namespace dawn::utils {

struct android_app* AndroidWindow::sApp = nullptr;

AndroidWindow::AndroidWindow() {}

AndroidWindow::~AndroidWindow() {}

wgpu::Surface AndroidWindow::CreateSurface(const wgpu::Instance& instance) {
    while (sApp->window == nullptr) {
        PollEvents();
    }

    wgpu::SurfaceDescriptorFromAndroidNativeWindow awDesc;
    awDesc.window = sApp->window;
    wgpu::SurfaceDescriptor surfaceDesc;
    surfaceDesc.nextInChain = &awDesc;

    return instance.CreateSurface(&surfaceDesc);
}

bool AndroidWindow::ShouldClose() {
    return mShouldClose;
}

void AndroidWindow::PollEvents() {
    LOGV("AndroidWindow::PollEvents");
    int events;
    android_poll_source* source = nullptr;
    if (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) < 0) {
        mShouldClose = true;
    } else if (source != nullptr) {
        source->process(sApp, source);
    }
}

// static
Window* Window::Create(uint32_t width, uint32_t height, const char* title) {
    return new AndroidWindow();
}

// static
void AndroidWindow::Init(struct android_app* app) {
    sApp = app;
}

}  // namespace dawn::utils
