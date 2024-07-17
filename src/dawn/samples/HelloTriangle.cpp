// Copyright 2017 The Dawn & Tint Authors
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

#include <vector>

#include "dawn/samples/SampleUtils.h"

#include "dawn/common/Log.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/SystemUtils.h"
#include "dawn/utils/WGPUHelpers.h"
#include "dawn/webgpu_cpp_print.h"

namespace wgpu {

using QueueWorkDoneCallback_ = void (*)(wgpu::QueueWorkDoneStatus status, void* userdata);

struct QueueWorkDoneCallbackInfo_ {
    template <typename F,
              typename T,
              typename Cb = void(QueueWorkDoneStatus status, T userdata),
              typename = std::enable_if_t<std::is_convertible_v<F, Cb*>>>
    QueueWorkDoneCallbackInfo_(CallbackMode callbackMode, F callback, T userdata)
        : mode(callbackMode),
          cCallback([](WGPUQueueWorkDoneStatus status, void* callback, void* userdata) {
              auto cb = reinterpret_cast<Cb*>(callback);
              (*cb)(static_cast<QueueWorkDoneStatus>(status), static_cast<T>(userdata));
          }),
          callback(reinterpret_cast<void*>(+callback)),
          userdata(reinterpret_cast<void*>(userdata)) {}

    template <typename L,
              typename F = void (*)(QueueWorkDoneStatus),
              typename = std::enable_if_t<std::is_convertible_v<L, F>>>
    QueueWorkDoneCallbackInfo_(CallbackMode callbackMode, L callbackIn)
        : mode(callbackMode),
          cCallback([](WGPUQueueWorkDoneStatus status, void* callback, void*) {
              auto cb = reinterpret_cast<F>(callback);
              (*cb)(static_cast<QueueWorkDoneStatus>(status));
          }),
          callback(reinterpret_cast<void*>(+callbackIn)),
          userdata(nullptr) {}

    ChainedStruct const* nextInChain = nullptr;
    const CallbackMode mode = CallbackMode::WaitAnyOnly;
    const WGPUQueueWorkDoneCallback2 cCallback =
        [](WGPUQueueWorkDoneStatus status, void* callback, void* userdata) {
            auto cb = reinterpret_cast<QueueWorkDoneCallback_>(callback);
            (*cb)(static_cast<QueueWorkDoneStatus>(status), userdata);
        };
    const void* callback = nullptr;
    const void* userdata = nullptr;
};

}  // namespace wgpu

wgpu::Device device;
wgpu::Queue queue;
wgpu::SwapChain swapchain;

wgpu::Buffer vertexBuffer;
wgpu::RenderPipeline pipeline;

wgpu::Future FakeQueueOnSubmittedWorkDone(wgpu::QueueWorkDoneCallbackInfo_ info) {
    static_assert(sizeof(wgpu::QueueWorkDoneCallbackInfo_) ==
                  sizeof(WGPUQueueWorkDoneCallbackInfo2));
    static_assert(offsetof(wgpu::QueueWorkDoneCallbackInfo_, nextInChain) ==
                  offsetof(WGPUQueueWorkDoneCallbackInfo2, nextInChain));
    static_assert(offsetof(wgpu::QueueWorkDoneCallbackInfo_, mode) ==
                  offsetof(WGPUQueueWorkDoneCallbackInfo2, mode));
    static_assert(offsetof(wgpu::QueueWorkDoneCallbackInfo_, cCallback) ==
                  offsetof(WGPUQueueWorkDoneCallbackInfo2, callback));
    static_assert(offsetof(wgpu::QueueWorkDoneCallbackInfo_, callback) ==
                  offsetof(WGPUQueueWorkDoneCallbackInfo2, userdata1));
    static_assert(offsetof(wgpu::QueueWorkDoneCallbackInfo_, userdata) ==
                  offsetof(WGPUQueueWorkDoneCallbackInfo2, userdata2));

    WGPUQueueWorkDoneCallbackInfo2 cInfo;
    memcpy(&cInfo, &info, sizeof(cInfo));
    return {wgpuQueueOnSubmittedWorkDone2(queue.Get(), cInfo).id};
}

void init() {
    device = CreateCppDawnDevice();
    queue = device.GetQueue();
    swapchain = GetSwapChain();

    static const float vertexData[12] = {
        0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f,
    };
    vertexBuffer = dawn::utils::CreateBufferFromData(device, vertexData, sizeof(vertexData),
                                                     wgpu::BufferUsage::Vertex);

    wgpu::ShaderModule module = dawn::utils::CreateShaderModule(device, R"(
        @vertex fn vs(@location(0) pos : vec4f) -> @builtin(position) vec4f {
            return pos;
        }

        @fragment fn fs(@builtin(position) FragCoord : vec4f) -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )");

    dawn::utils::ComboRenderPipelineDescriptor descriptor;
    descriptor.layout = nullptr;
    descriptor.vertex.module = module;
    descriptor.vertex.bufferCount = 1;
    descriptor.cBuffers[0].arrayStride = 4 * sizeof(float);
    descriptor.cBuffers[0].attributeCount = 1;
    descriptor.cAttributes[0].format = wgpu::VertexFormat::Float32x4;
    descriptor.cFragment.module = module;
    descriptor.cTargets[0].format = GetPreferredSwapChainTextureFormat();

    pipeline = device.CreateRenderPipeline(&descriptor);
}

static int sUserdata = 1;
void frame() {
    wgpu::TextureView backbufferView = swapchain.GetCurrentTextureView();
    dawn::utils::ComboRenderPassDescriptor renderPass({backbufferView});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.Draw(3);
        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    swapchain.Present();

    FakeQueueOnSubmittedWorkDone(
        {wgpu::CallbackMode::WaitAnyOnly, [](wgpu::QueueWorkDoneStatus status) {
             DAWN_DEBUG() << "Worked with lambda " << status;
         }});

    FakeQueueOnSubmittedWorkDone({wgpu::CallbackMode::WaitAnyOnly,
                                  [](wgpu::QueueWorkDoneStatus status, int* userdata) {
                                      DAWN_DEBUG() << "Worked with lambda " << status
                                                   << " userdata " << *userdata;
                                  },
                                  &sUserdata});

    DoFlush();
}

int main(int argc, const char* argv[]) {
    if (!InitSample(argc, argv)) {
        return 1;
    }
    init();

    while (!ShouldQuit()) {
        frame();
        dawn::utils::USleep(16000);
    }
}
