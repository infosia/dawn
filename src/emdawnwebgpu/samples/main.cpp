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

#include <emscripten.h>
#include <emscripten/html5.h>
#include <webgpu/webgpu_cpp.h>

#include <cassert>
#include <cstdio>
#include <utility>

static wgpu::Instance instance;
static wgpu::Adapter adapter;
static wgpu::Device device;
static wgpu::Queue queue;
static wgpu::RenderPipeline pipeline;
static wgpu::Buffer uniformBuffer;
static wgpu::Surface surface;

static int frameNum = 0;

// This is HTML's default canvas size.
static constexpr uint32_t kWidth = 300;
static constexpr uint32_t kHeight = 150;

wgpu::Device GetDevice() {
    wgpu::Future f1 = instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestAdapterStatus status, wgpu::Adapter ad, wgpu::StringView message) {
            if (message.length) {
                printf("RequestAdapter: %.*s\n", static_cast<int>(message.length), message.data);
            }
            if (status == wgpu::RequestAdapterStatus::Unavailable) {
                printf("WebGPU unavailable; exiting cleanly\n");
                exit(0);
            }
            assert(status == wgpu::RequestAdapterStatus::Success);

            adapter = std::move(ad);
        });
    instance.WaitAny(f1, UINT64_MAX);
    assert(adapter);

    wgpu::DeviceDescriptor desc;
    desc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
            printf("UncapturedError (errorType=%d): %.*s\n", errorType,
                   static_cast<int>(message.length), message.data);
            assert(false);
        });
    desc.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
            printf("DeviceLost (reason=%d): %.*s\n", reason, static_cast<int>(message.length),
                   message.data);
        });

    wgpu::Device device;
    wgpu::Future f2 = adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestDeviceStatus status, wgpu::Device dev, wgpu::StringView message) {
            if (message.length) {
                printf("RequestDevice: %.*s\n", static_cast<int>(message.length), message.data);
            }
            assert(status == wgpu::RequestDeviceStatus::Success);

            device = std::move(dev);
        });
    instance.WaitAny(f2, UINT64_MAX);
    assert(device);

    return device;
}

static const char shaderCode[] = R"(
    @vertex
    fn main_v(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4<f32> {
        var pos = array<vec2f, 3>(
            vec2f(0.0, 0.5), vec2f(-0.5, -0.5), vec2f(0.5, -0.5));
        return vec4f(pos[idx], 0, 1);
    }
    @fragment
    fn main_f() -> @location(0) vec4<f32> {
        return vec4f(0, 1, 0, 1);
    }
)";

void init() {
    static wgpu::TextureFormat preferredFormat;
    {
        wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
        canvasDesc.selector = "#canvas";

        wgpu::SurfaceDescriptor surfDesc{};
        surfDesc.nextInChain = &canvasDesc;
        surface = instance.CreateSurface(&surfDesc);

        {
            wgpu::SurfaceCapabilities caps;
            surface.GetCapabilities(adapter, &caps);
            preferredFormat = caps.formats[0];
        }

        wgpu::SurfaceConfiguration configuration{};
        configuration.device = device;
        configuration.usage = wgpu::TextureUsage::RenderAttachment;
        configuration.format = preferredFormat;
        configuration.width = kWidth;
        configuration.height = kHeight;
        configuration.alphaMode = wgpu::CompositeAlphaMode::Premultiplied;
        configuration.presentMode = wgpu::PresentMode::Fifo;
        surface.Configure(&configuration);
    }

    queue = device.GetQueue();

    wgpu::ShaderModule shaderModule{};
    {
        wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
        wgslDesc.code = shaderCode;

        wgpu::ShaderModuleDescriptor descriptor{};
        descriptor.nextInChain = &wgslDesc;
        shaderModule = device.CreateShaderModule(&descriptor);
    }

    {
        wgpu::ColorTargetState colorTargetState{};
        colorTargetState.format = preferredFormat;

        wgpu::FragmentState fragmentState{};
        fragmentState.module = shaderModule;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTargetState;

        wgpu::RenderPipelineDescriptor descriptor{};
        descriptor.layout = nullptr;
        descriptor.vertex.module = shaderModule;
        descriptor.fragment = &fragmentState;
        descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

        pipeline = device.CreateRenderPipeline(&descriptor);
    }
}

void render(wgpu::TextureView view) {
    wgpu::RenderPassColorAttachment attachment{};
    attachment.view = view;
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = {0, 0, static_cast<float>(frameNum % 150) / 149, 0.5};

    wgpu::RenderPassDescriptor renderpass{};
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
            pass.SetPipeline(pipeline);
            pass.Draw(3);
            pass.End();
        }
        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);
}

// Export `frame` so it can be used with `ccall`, below.
extern "C" {
EMSCRIPTEN_KEEPALIVE bool frame();
}

bool frame() {
    frameNum++;

    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);
    assert(surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Success);
    wgpu::TextureView backbuffer = surfaceTexture.texture.CreateView();

    render(backbuffer);

    return true;  // Continue the requestAnimationFrame loop
}

int main() {
    printf("Initializing...\n");
    wgpu::InstanceDescriptor desc;
    desc.capabilities.timedWaitAnyEnable = true;
    instance = wgpu::CreateInstance(&desc);
    device = GetDevice();

    init();

    // Workaround for JSPI not working in emscripten_set_main_loop. Loosely based on this code:
    // https://github.com/emscripten-core/emscripten/issues/22493#issuecomment-2330275282
    // Note the following link args are required:
    // - JSPI: -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$getWasmTableEntry
    // - Asyncify: -sEXPORTED_RUNTIME_METHODS=ccall
    // clang-format off
    EM_ASM({
#if SAMPLE_USE_JSPI
        // -sJSPI=1 (aka -sASYNCIFY=2)
        var callback = WebAssembly.promising(getWasmTableEntry($0));
#else
        // -sASYNCIFY=1
        // ccall seems to be the only thing in Emscripten which lets us turn an
        // Asyncified Wasm function into a JS function returning a Promise.
        // It can only call exported functions, so we must export `frame`.
        var callback = () =>
            globalThis['Module']['ccall']("frame", "boolean", [], [], { async: true });
#endif

        async function tick() {
            // Start the frame callback. 'await' means we won't call
            // requestAnimationFrame again until it completes.
            var keepLooping = await callback();
            if (keepLooping) {
                requestAnimationFrame(tick);
            }
        }
        requestAnimationFrame(tick);
    }, frame);
    // clang-format on
}
