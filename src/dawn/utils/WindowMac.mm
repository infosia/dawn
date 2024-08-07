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

#include "dawn/utils/WindowMac.h"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "dawn/common/Log.h"

bool gRunning = true;

namespace dawn::utils {

Window::Window(NSWindow* window, CAMetalLayer* layer) : mNSWindow(window), mLayer(layer) {}

Window::~Window() {
    [mNSWindow close];
}

wgpu::Surface Window::CreateSurface(const wgpu::Instance& instance) {
    wgpu::SurfaceDescriptorFromMetalLayer desc;
    desc.layer = mLayer;

    wgpu::SurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &desc;
    return instance.CreateSurface(&surfaceDescriptor);
}

bool Window::ShouldClose() {
    return !gRunning;
}

void Window::PollEvents() {
    NSApplication* app = [NSApplication sharedApplication];
    NSEvent* nsEvent = [app nextEventMatchingMask:NSEventMaskAny
                                        untilDate:nil
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    [NSApp sendEvent:nsEvent];
}

void Window::WaitEvents() {
    NSApplication* app = [NSApplication sharedApplication];
    NSEvent* nsEvent = [app nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    [NSApp sendEvent:nsEvent];
}

// static
std::unique_ptr<Window> Window::Create(uint32_t width, uint32_t height, const char* title) {
    if (![[NSRunningApplication currentApplication] isFinishedLaunching]) {
        [NSApp run];
    }

    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect rect = NSMakeRect(0, 0, width, height);
    int mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
               NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSWindow* window = [[NSWindow alloc] initWithContentRect:rect
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    if (!window) {
        return nullptr;
    }
    [window makeKeyAndOrderFront:NSApp];

    id<MTLDevice> mtlDevice = MTLCreateSystemDefaultDevice();

    if (!mtlDevice) {
        return nullptr;
    }

    CGSize size;
    size.width = width;
    size.height = height;

    CAMetalLayer* layer = [CAMetalLayer layer];
    if (!layer) {
        return nullptr;
    }

    [layer setDevice:mtlDevice];
    [layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
    [layer setFramebufferOnly:YES];
    [layer setDrawableSize:size];
    [layer setColorspace:CGColorSpaceCreateDeviceRGB()];

    NSView* view = [[NSView alloc] initWithFrame:rect];
    [view setWantsLayer:YES];
    [view setLayer:layer];

    [window setContentView:view];

    return std::make_unique<Window>(window, layer);
}

}  // namespace dawn::utils
