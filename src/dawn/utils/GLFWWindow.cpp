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

#include "dawn/utils/GLFWWindow.h"

#include "GLFW/glfw3.h"

#include "dawn/common/Log.h"
#include "webgpu/webgpu_glfw.h"

namespace dawn::utils {

GLFWWindow::GLFWWindow(GLFWwindow* window) : mWindow(window) {}

GLFWWindow::~GLFWWindow() {
    glfwDestroyWindow(mWindow);
}

// static
Window* Window::Create(uint32_t width, uint32_t height, const char* title) {
    static bool initialized;
    if (!initialized) {
        glfwSetErrorCallback([](int code, const char* message) {
            dawn::ErrorLog() << "GLFW error: " << code << " - " << message;
        });
        if (!glfwInit()) {
            return nullptr;
        }
        initialized = true;
    }
    // Set GLFW_NO_API to avoid GLFW bringing up a GL context that we won't use.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        return nullptr;
    }
    return new GLFWWindow(window);
}

wgpu::Surface GLFWWindow::CreateSurface(const wgpu::Instance& instance) {
    return wgpu::glfw::CreateSurfaceForWindow(instance, mWindow);
}

bool GLFWWindow::ShouldClose() {
    return glfwWindowShouldClose(mWindow);
}

void GLFWWindow::PollEvents() {
    glfwPollEvents();
}

}  // namespace dawn::utils
