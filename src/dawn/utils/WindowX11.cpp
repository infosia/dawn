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

#include "dawn/utils/WindowX11.h"

#include "dawn/common/Log.h"

bool gRunning = true;

namespace dawn::utils {

Window::Window(Display* display, ::Window window) : mDisplay(display), mWindow(window) {}

Window::~Window() {
    if (mWindow) {
        XUnmapWindow(mDisplay, mWindow);
        XDestroyWindow(mDisplay, mWindow);
    }
}

wgpu::Surface Window::CreateSurface(const wgpu::Instance& instance) {
    wgpu::SurfaceDescriptorFromXlibWindow desc;
    desc.display = mDisplay;
    desc.window = mWindow;

    wgpu::SurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &desc;
    return instance.CreateSurface(&surfaceDescriptor);
}

bool Window::ShouldClose() {
    return !gRunning;
}

void Window::PollEvents() {
    if (XPending(mDisplay)) {
        XEvent event;
        XNextEvent(mDisplay, &event);
    }
}

void Window::WaitEvents() {
    XEvent event;
    XNextEvent(mDisplay, &event);
}

// static
std::unique_ptr<Window> Window::Create(uint32_t width, uint32_t height, const char* title) {
    Display* display = XOpenDisplay(0);
    if (!display) {
        return nullptr;
    }
    ::Window rootWindow = RootWindow(display, DefaultScreen(display));

    XVisualInfo visualInfo;
    if (!XMatchVisualInfo(display, DefaultScreen(display), 24, TrueColor, &visualInfo)) {
        return nullptr;
    }

    Colormap colorMap = XCreateColormap(display, rootWindow, visualInfo.visual, AllocNone);
    XSetWindowAttributes windowAttributes;
    windowAttributes.border_pixel = BlackPixel(display, visualInfo.screen);
    windowAttributes.colormap = colorMap;
    windowAttributes.event_mask = StructureNotifyMask;
    ::Window window = XCreateWindow(display, rootWindow, 0, 0, width, height, 1, /* border_width */
                                    visualInfo.depth, InputOutput, visualInfo.visual,
                                    CWColormap | CWEventMask, &windowAttributes);
    if (!window) {
        return nullptr;
    }

    XMapWindow(display, window);

    return std::make_unique<Window>(display, window);
}

}  // namespace dawn::utils
