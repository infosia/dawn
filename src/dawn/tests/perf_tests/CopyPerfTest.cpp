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

#include <cstdlib>
#include <vector>

#include "dawn/common/Math.h"
#include "dawn/tests/perf_tests/DawnPerfTest.h"
#include "dawn/utils/TestUtils.h"
#include "dawn/utils/TextureUtils.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

constexpr unsigned int kNumIterations = 100;
constexpr unsigned int kWidth = 4 * 1024;
constexpr unsigned int kHeight = 4 * 1024;
constexpr unsigned int kUnalignedOffset = 3;
constexpr unsigned int kNumBuffers = 10;

// Test copy a texture to a buffer |kNumIterations| times.
class CopyPerfTest : public DawnPerfTest {
  public:
    CopyPerfTest() : DawnPerfTest(kNumIterations, 10) {}
    ~CopyPerfTest() override = default;

    void SetUp() override;

  private:
    void Step() override;

    wgpu::Buffer dstBuffers[kNumBuffers];
    wgpu::Texture srcTextures[kNumBuffers];
};

void CopyPerfTest::SetUp() {
    DawnPerfTest::SetUp();

    wgpu::Buffer stagingBuffer;
    {
        wgpu::BufferDescriptor desc = {};
        desc.size = kWidth * kHeight;
        desc.usage = wgpu::BufferUsage::CopySrc;
        desc.mappedAtCreation = true;
        stagingBuffer = device.CreateBuffer(&desc);

        auto dstPtr = static_cast<int8_t*>(stagingBuffer.GetMappedRange());
        for (size_t i = 0; i < desc.size; ++i) {
            dstPtr[i] = rand() % 128;
        }
        stagingBuffer.Unmap();
    }

    for (auto& src : srcTextures) {
        wgpu::TextureDescriptor textureDescriptor;
        textureDescriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        textureDescriptor.format = wgpu::TextureFormat::R8Snorm;
        textureDescriptor.sampleCount = 1;
        textureDescriptor.size.width = kWidth;
        textureDescriptor.size.height = kHeight;

        src = device.CreateTexture(&textureDescriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        wgpu::TexelCopyBufferInfo imageCopyBuffer =
            utils::CreateTexelCopyBufferInfo(stagingBuffer, 0, kWidth, kHeight);
        wgpu::TexelCopyTextureInfo imageCopyTexture = utils::CreateTexelCopyTextureInfo(src, 0, {});

        wgpu::Extent3D copySize{kWidth, kHeight, 1};

        encoder.CopyBufferToTexture(&imageCopyBuffer, &imageCopyTexture, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    for (auto& dst : dstBuffers) {
        wgpu::BufferDescriptor desc = {};
        desc.size = Align(kUnalignedOffset + kWidth * kHeight, 4);
        desc.usage = wgpu::BufferUsage::CopyDst;

        dst = device.CreateBuffer(&desc);
    }
}

void CopyPerfTest::Step() {
    for (unsigned int i = 0; i < kNumIterations; ++i) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        for (unsigned int j = 0; j < kNumBuffers; ++j) {
            auto& src = srcTextures[j];
            auto& dst = dstBuffers[j];

            wgpu::TexelCopyBufferInfo imageCopyBuffer =
                utils::CreateTexelCopyBufferInfo(dst, kUnalignedOffset, kWidth, kHeight);
            wgpu::ImageCopyTexture imageCopyTexture = utils::CreateTexelCopyTextureInfo(src, 0, {});
            wgpu::Extent3D copySize{kWidth, kHeight, 1};

            encoder.CopyTextureToBuffer(&imageCopyTexture, &imageCopyBuffer, &copySize);
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }
}

TEST_P(CopyPerfTest, Run) {
    RunTest();
}

DAWN_INSTANTIATE_TEST(CopyPerfTest,
                      D3D11Backend(),
                      D3D11Backend({}, {"use_blit_for_t2b"}),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
