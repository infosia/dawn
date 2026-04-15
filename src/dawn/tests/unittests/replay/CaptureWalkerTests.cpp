// Copyright 2026 The Dawn & Tint Authors
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "src/dawn/replay/CaptureWalker.h"
#include "src/dawn/replay/ReadHead.h"
#include "src/dawn/replay/SurfaceDiscovery.h"

namespace dawn::replay {
namespace {

using testing::ElementsAre;

class MockResourceVisitor : public ResourceVisitor {
  public:
    MaybeError operator()(const BindGroupData&) override { return VisitBindGroup(); }
    MaybeError operator()(const BindGroupLayoutData&) override { return VisitBindGroupLayout(); }
    MaybeError operator()(const schema::Buffer&) override { return VisitBuffer(); }
    MaybeError operator()(const CommandBufferData& data) override {
        return VisitCommandBuffer(data);
    }
    MaybeError operator()(const schema::ComputePipeline&) override {
        return VisitComputePipeline();
    }
    MaybeError operator()(const DeviceData&) override { return VisitDevice(); }
    MaybeError operator()(const schema::ExternalTexture&) override {
        return VisitExternalTexture();
    }
    MaybeError operator()(const schema::PipelineLayout&) override { return VisitPipelineLayout(); }
    MaybeError operator()(const schema::QuerySet&) override { return VisitQuerySet(); }
    MaybeError operator()(const RenderBundleData& data) override { return VisitRenderBundle(data); }
    MaybeError operator()(const schema::RenderPipeline&) override { return VisitRenderPipeline(); }
    MaybeError operator()(const schema::Sampler&) override { return VisitSampler(); }
    MaybeError operator()(const schema::ShaderModule&) override { return VisitShaderModule(); }
    MaybeError operator()(const InvalidData&) override { return VisitInvalid(); }
    MaybeError operator()(const schema::TexelBufferView&) override {
        return VisitTexelBufferView();
    }
    MaybeError operator()(const schema::Texture&) override { return VisitTexture(); }
    MaybeError operator()(const schema::TextureView&) override { return VisitTextureView(); }
    MaybeError operator()(const std::monostate&) override { return VisitMonostate(); }

    MOCK_METHOD(MaybeError, VisitBindGroup, ());
    MOCK_METHOD(MaybeError, VisitBindGroupLayout, ());
    MOCK_METHOD(MaybeError, VisitBuffer, ());
    MOCK_METHOD(MaybeError, VisitCommandBuffer, (const CommandBufferData&));
    MOCK_METHOD(MaybeError, VisitComputePipeline, ());
    MOCK_METHOD(MaybeError, VisitDevice, ());
    MOCK_METHOD(MaybeError, VisitExternalTexture, ());
    MOCK_METHOD(MaybeError, VisitPipelineLayout, ());
    MOCK_METHOD(MaybeError, VisitQuerySet, ());
    MOCK_METHOD(MaybeError, VisitRenderBundle, (const RenderBundleData&));
    MOCK_METHOD(MaybeError, VisitRenderPipeline, ());
    MOCK_METHOD(MaybeError, VisitSampler, ());
    MOCK_METHOD(MaybeError, VisitShaderModule, ());
    MOCK_METHOD(MaybeError, VisitInvalid, ());
    MOCK_METHOD(MaybeError, VisitTexelBufferView, ());
    MOCK_METHOD(MaybeError, VisitTexture, ());
    MOCK_METHOD(MaybeError, VisitTextureView, ());
    MOCK_METHOD(MaybeError, VisitMonostate, ());
};

class MockRootCommandVisitor : public RootCommandVisitor {
  public:
    MOCK_METHOD(void, SetContentReadHead, (ReadHead * readHead), (override));
    MOCK_METHOD(ResourceVisitor&, GetResourceVisitor, (), (override));

    MaybeError operator()(const CreateResourceData& data) override {
        return VisitCreateResource(data);
    }
    MaybeError operator()(const schema::RootCommandWriteBufferCmdData& data) override {
        return VisitWriteBuffer(data);
    }
    MaybeError operator()(const schema::RootCommandWriteTextureCmdData& data) override {
        return VisitWriteTexture(data);
    }
    MaybeError operator()(const schema::RootCommandQueueSubmitCmdData& data) override {
        return VisitQueueSubmit(data);
    }
    MaybeError operator()(const schema::RootCommandSetLabelCmdData& data) override {
        return VisitSetLabel(data);
    }
    MaybeError operator()(const schema::RootCommandInitTextureCmdData& data) override {
        return VisitInitTexture(data);
    }
    MaybeError operator()(const schema::RootCommandSurfaceConfigureCmdData& data) override {
        return VisitSurfaceConfigure(data);
    }
    MaybeError operator()(const schema::RootCommandSurfaceUnconfigureCmdData& data) override {
        return VisitSurfaceUnconfigure(data);
    }
    MaybeError operator()(const schema::RootCommandSurfacePresentCmdData& data) override {
        return VisitSurfacePresent(data);
    }
    MaybeError operator()(const schema::RootCommandSurfaceGetCurrentTextureCmdData& data) override {
        return VisitSurfaceGetCurrentTexture(data);
    }
    MaybeError operator()(const schema::RootCommandEndCmdData& data) override {
        return VisitEnd(data);
    }
    MaybeError operator()(const std::monostate& data) override { return VisitMonostate(data); }

    MOCK_METHOD(MaybeError, VisitCreateResource, (const CreateResourceData& data));
    MOCK_METHOD(MaybeError, VisitWriteBuffer, (const schema::RootCommandWriteBufferCmdData& data));
    MOCK_METHOD(MaybeError,
                VisitWriteTexture,
                (const schema::RootCommandWriteTextureCmdData& data));
    MOCK_METHOD(MaybeError, VisitQueueSubmit, (const schema::RootCommandQueueSubmitCmdData& data));
    MOCK_METHOD(MaybeError, VisitSetLabel, (const schema::RootCommandSetLabelCmdData& data));
    MOCK_METHOD(MaybeError, VisitInitTexture, (const schema::RootCommandInitTextureCmdData& data));
    MOCK_METHOD(MaybeError,
                VisitSurfaceConfigure,
                (const schema::RootCommandSurfaceConfigureCmdData& data));
    MOCK_METHOD(MaybeError,
                VisitSurfaceUnconfigure,
                (const schema::RootCommandSurfaceUnconfigureCmdData& data));
    MOCK_METHOD(MaybeError,
                VisitSurfacePresent,
                (const schema::RootCommandSurfacePresentCmdData& data));
    MOCK_METHOD(MaybeError,
                VisitSurfaceGetCurrentTexture,
                (const schema::RootCommandSurfaceGetCurrentTextureCmdData& data));
    MOCK_METHOD(MaybeError, VisitEnd, (const schema::RootCommandEndCmdData& data));
    MOCK_METHOD(MaybeError, VisitMonostate, (const std::monostate&));
};

class TestCaptureWalker : public CaptureWalker {
  public:
    explicit TestCaptureWalker(std::vector<uint8_t> commands) : mCommands(std::move(commands)) {}

  protected:
    ReadHead GetCommandReadHead() const override { return ReadHead(mCommands); }
    ReadHead GetContentReadHead() const override { return ReadHead(mContent); }

  private:
    std::vector<uint8_t> mCommands;
    std::vector<uint8_t> mContent;
};

// Test that CaptureWalker correctly skips nested CommandBuffer commands when using
// SurfaceDiscoveryVisitor.
TEST(CaptureWalkerTests, SurfaceDiscoverySkipsCommandBuffer) {
    std::vector<uint8_t> commands;

    auto Emit = [&](auto v) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
        commands.insert(commands.end(), p, p + sizeof(v));
    };

    auto EmitString = [&](std::string s) {
        Emit(static_cast<size_t>(s.size()));
        commands.insert(commands.end(), s.begin(), s.end());
    };

    // 1. CreateResource (CommandBuffer)
    Emit(schema::RootCommand::CreateResource);
    // LabeledResource
    Emit(schema::ObjectType::CommandBuffer);  // type
    Emit(static_cast<schema::ObjectId>(10));  // id
    EmitString("MyCommandBuffer");            // label
    // CommandBufferData (empty, but nested commands follow)
    // Nested Command: CopyBufferToBuffer
    Emit(schema::CommandBufferCommand::CopyBufferToBuffer);
    Emit(static_cast<schema::ObjectId>(1));  // src
    Emit(static_cast<uint64_t>(0));          // srcOffset
    Emit(static_cast<schema::ObjectId>(2));  // dst
    Emit(static_cast<uint64_t>(0));          // dstOffset
    Emit(static_cast<uint64_t>(1024));       // size
    // Nested Command: End
    Emit(schema::CommandBufferCommand::End);

    // 2. SurfaceConfigure (Surface) - This is what we want to discover
    Emit(schema::RootCommand::SurfaceConfigure);
    Emit(static_cast<schema::ObjectId>(20));  // surfaceId
    // SurfaceConfiguration
    Emit(static_cast<schema::ObjectId>(1));      // deviceId
    Emit(wgpu::TextureFormat::RGBA8Unorm);       // format
    Emit(wgpu::TextureUsage::RenderAttachment);  // usage
    Emit(static_cast<size_t>(0));                // viewFormats count
    Emit(wgpu::CompositeAlphaMode::Opaque);      // alphaMode
    Emit(static_cast<uint32_t>(100));            // width
    Emit(static_cast<uint32_t>(200));            // height
    Emit(wgpu::PresentMode::Fifo);               // presentMode

    // 3. End
    Emit(schema::RootCommand::End);

    TestCaptureWalker walker(commands);
    SurfaceDiscoveryVisitor visitor;
    MaybeError result = walker.Walk(visitor);
    EXPECT_FALSE(result.IsError());

    std::vector<schema::ObjectId> surfaceIds = visitor.GetSurfaceIds();
    EXPECT_THAT(surfaceIds, ElementsAre(20));
}

}  // anonymous namespace
}  // namespace dawn::replay
