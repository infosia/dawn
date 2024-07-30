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

#include "dawn/utils/WindowWin.h"

#include <windows.h>

#include "dawn/common/Log.h"

bool gRunning = true;

namespace dawn::utils {

Window::Window(HWND window) : mHwnd(window) {}

Window::~Window() {
    if (mHwnd) {
        DestroyWindow(mHwnd);
    }
}

wgpu::Surface Window::CreateSurface(const wgpu::Instance& instance) {
    wgpu::SurfaceDescriptorFromWindowsHWND desc;
    desc.hwnd = mHwnd;
    desc.hinstance = GetModuleHandle(nullptr);

    wgpu::SurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &desc;
    return instance.CreateSurface(&surfaceDescriptor);
}

bool Window::ShouldClose() {
    return !gRunning;
}

void Window::PollEvents() {
    MSG msg;
    while (PeekMessage(&msg, mHwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Window::WaitEvents() {
    MSG msg;
    while (GetMessage(&msg, mHwnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LONG rc = 0L;

    switch (message) {
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            gRunning = false;
            break;
        case WM_SIZE:
            break;
        default:
            rc = DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }
    return rc;
}

static void registerMainWindowClass() {
    static bool registered = false;
    if (!registered) {
        WNDCLASS wc;
        wc.lpszClassName = L"dawnSampleWindowClass";
        wc.lpfnWndProc = wndProc;
        wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
        wc.hInstance = nullptr;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        wc.lpszMenuName = L"mainMenu";
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;

        RegisterClass(&wc);
        registered = true;
    }
}

// static
std::unique_ptr<Window> Window::Create(uint32_t width, uint32_t height, const char* title) {
    registerMainWindowClass();

    RECT r = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    DWORD style = WS_OVERLAPPEDWINDOW;
    bool hasMenuBar = false;
    AdjustWindowRect(&r, style, hasMenuBar);
    HWND hwnd = CreateWindowA("dawnSampleWindowClass", title, style, 0, 0, r.right - r.left,
                              r.bottom - r.top, nullptr, nullptr, nullptr, nullptr);
    if (!hwnd) {
        return nullptr;
    }

    ShowWindow(hwnd, SW_SHOW);

    return std::make_unique<Window>(hwnd);
}

}  // namespace dawn::utils
