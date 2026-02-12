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

#ifndef SRC_DAWN_REPLAY_REPLAYIMPL_H_
#define SRC_DAWN_REPLAY_REPLAYIMPL_H_

#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "dawn/replay/BlitBufferToDepthTexture.h"
#include "dawn/replay/Capture.h"
#include "dawn/replay/Replay.h"
#include "dawn/serialization/Schema.h"
#include "dawn/webgpu_cpp.h"

namespace dawn::replay {

// These x-macros use DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES to generate
// an std::variant that includes each type of BindGroupLayoutEntry. This
// std::variant can then be used in CreateBindGroupLayout below with
// a visitor to separate deserialization from actual use.
#define DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_VALID(NAME) schema::BindGroupLayoutEntryType##NAME,
#define DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_INVALID(NAME, VALUE)
#define DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_GET_MACRO(_1, _2, NAME, ...) NAME
#define DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE(...)                  \
    DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_GET_MACRO(                \
        __VA_ARGS__, DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_INVALID, \
        DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_VALID)(__VA_ARGS__)

using BindGroupLayoutEntryVariant = std::variant<DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES(
    DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE) std::monostate>;
#undef DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE
#undef DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_GET_MACRO
#undef DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_INVALID
#undef DAWN_REPLAY_BINDGROUPLAYOUT_VARIANT_TYPE_VALID

// These x-macros use DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES which
// is a list of all BindGroupLayoutEntry types to auto generate
// a switch case for each type of BindGroupLayoutEntryType that deserializes
// a capture for that type and converts it to an std::variant entry
// for that type.
#define DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_VALID(NAME) schema::BindGroupEntryType##NAME,
#define DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_INVALID(NAME, VALUE)
#define DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_GET_MACRO(_1, _2, NAME, ...) NAME
#define DAWN_REPLAY_BINDGROUP_VARIANT_TYPE(...)                  \
    DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_GET_MACRO(                \
        __VA_ARGS__, DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_INVALID, \
        DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_VALID)(__VA_ARGS__)

#define DAWN_REPLAY_GEN_BINDGROUP_VARIANT(ENUM_NAME, MEMBERS) \
    using BindGroupEntryVariant =                             \
        std::variant<MEMBERS(DAWN_REPLAY_BINDGROUP_VARIANT_TYPE) std::monostate>;

DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES_ENUM(DAWN_REPLAY_GEN_BINDGROUP_VARIANT)

#undef DAWN_REPLAY_GEN_BINDGROUP_VARIANT
#undef DAWN_REPLAY_BINDGROUP_VARIANT_TYPE
#undef DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_GET_MACRO
#undef DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_INVALID
#undef DAWN_REPLAY_BINDGROUP_VARIANT_TYPE_VALID

// These structures are used to gather data in a generic way to pass to
// visitors for resource creation. For example, BindGroupData is used to
// select the bindGroupEntries that, as serialized are different types,
// and deserialize them into an std::variant of the types that can then
// be passed to a visitor in a generic way. BindGroupLayoutData does
// the same for bindGroupLayoutEntries. For CommandBufferData and
// RenderBundleData, the deserialization is passed on to lower level
// functions and visitors. See DAWN_REPLAY_RESOURCE_DATA_MAP below.
struct InvalidData {};
struct DeviceData {};

struct BindGroupData {
    schema::BindGroup bg;
    std::vector<BindGroupEntryVariant> entries;
};

struct BindGroupLayoutData {
    schema::BindGroupLayout bgl;
    std::vector<BindGroupLayoutEntryVariant> entries;
};

class ReadHead;

struct CommandBufferData {
    ReadHead* readHead;
};

struct RenderBundleData {
    schema::RenderBundle bundle;
    ReadHead* readHead;
};

typedef std::variant<wgpu::BindGroup,
                     wgpu::BindGroupLayout,
                     wgpu::Buffer,
                     wgpu::CommandBuffer,
                     wgpu::ComputePipeline,
                     wgpu::Device,
                     wgpu::ExternalTexture,
                     wgpu::PipelineLayout,
                     wgpu::QuerySet,
                     wgpu::RenderBundle,
                     wgpu::RenderPipeline,
                     wgpu::Sampler,
                     wgpu::ShaderModule,
                     wgpu::TexelBufferView,
                     wgpu::Texture,
                     wgpu::TextureView>
    Resource;

struct LabeledResource {
    std::string label;
    Resource resource;
};

// This is needed to map a command to a deserialization data type.
// In particular, BindGroupData, BindGroupLayoutData, have
// packed variants, meaning, entry has a type enum, and then only
// the data needed for that particular variant. In order to use
// std::variant, these need to be expanded to an variant that is
// padded to the largest type.
#define DAWN_REPLAY_RESOURCE_DATA_MAP(X)        \
    X(Invalid, InvalidData)                     \
    X(BindGroup, BindGroupData)                 \
    X(BindGroupLayout, BindGroupLayoutData)     \
    X(Buffer, schema::Buffer)                   \
    X(CommandBuffer, CommandBufferData)         \
    X(ComputePipeline, schema::ComputePipeline) \
    X(Device, DeviceData)                       \
    X(ExternalTexture, schema::ExternalTexture) \
    X(PipelineLayout, schema::PipelineLayout)   \
    X(QuerySet, schema::QuerySet)               \
    X(RenderBundle, RenderBundleData)           \
    X(RenderPipeline, schema::RenderPipeline)   \
    X(Sampler, schema::Sampler)                 \
    X(ShaderModule, schema::ShaderModule)       \
    X(TexelBufferView, schema::TexelBufferView) \
    X(Texture, schema::Texture)                 \
    X(TextureView, schema::TextureView)

#define AS_DATA_TYPE(NAME, TYPE) TYPE,
using ResourceData = std::variant<DAWN_REPLAY_RESOURCE_DATA_MAP(AS_DATA_TYPE) std::monostate>;
#undef AS_DATA_TYPE

struct CreateResourceData {
    schema::LabeledResource resource;
    ResourceData data;
};

// ResourceData is an std::variant of the creation parameters
// of each type of resource. This can be used with std::visit
// to handle each type of resource.
class ResourceVisitor {
  public:
    virtual ~ResourceVisitor() = default;

    virtual MaybeError operator()(const BindGroupData& data) = 0;
    virtual MaybeError operator()(const BindGroupLayoutData& data) = 0;
    virtual MaybeError operator()(const schema::Buffer& data) = 0;
    virtual MaybeError operator()(const CommandBufferData& data) = 0;
    virtual MaybeError operator()(const schema::ComputePipeline& data) = 0;
    virtual MaybeError operator()(const schema::ExternalTexture& data) = 0;
    virtual MaybeError operator()(const schema::PipelineLayout& data) = 0;
    virtual MaybeError operator()(const schema::QuerySet& data) = 0;
    virtual MaybeError operator()(const RenderBundleData& data) = 0;
    virtual MaybeError operator()(const schema::RenderPipeline& data) = 0;
    virtual MaybeError operator()(const schema::Sampler& data) = 0;
    virtual MaybeError operator()(const schema::ShaderModule& data) = 0;
    virtual MaybeError operator()(const schema::TexelBufferView& data) = 0;
    virtual MaybeError operator()(const schema::Texture& data) = 0;
    virtual MaybeError operator()(const schema::TextureView& data) = 0;
    virtual MaybeError operator()(const InvalidData& data) = 0;
    virtual MaybeError operator()(const DeviceData& data) = 0;
};

class RootCommandVisitor {
  public:
    virtual ~RootCommandVisitor() = default;

    virtual void SetContentReadHead(ReadHead* readHead) = 0;
    virtual ResourceVisitor& GetResourceVisitor() = 0;
    virtual MaybeError operator()(const CreateResourceData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandWriteBufferCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandWriteTextureCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandQueueSubmitCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandSetLabelCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandInitTextureCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandUnmapBufferCmdData& data) = 0;
    virtual MaybeError operator()(const schema::RootCommandEndCmdData& data) = 0;
};

class ReplayImplBase {
  public:
    explicit ReplayImplBase(std::unique_ptr<const CaptureImpl> capture);
    MaybeError Play(RootCommandVisitor& visitor);

  private:
    std::unique_ptr<const CaptureImpl> mCapture;
};

class DawnRootCommandVisitor;

// Replays a capture. For now we only support replaying the entire capture.
// In the future we'd like to be able to replay up to a certain point.
class ReplayImpl : public Replay {
  public:
    static std::unique_ptr<ReplayImpl> Create(wgpu::Device device,
                                              std::unique_ptr<Capture> capture);

    MaybeError Play();

    // Gets the first object of type T with the given label, or nullptr if not
    // found. Note: We don't too much care this is slow as mostly used for
    // testing and debugging.
    template <typename T>
    T GetObjectByLabel(std::string_view label) const;

    template <typename T>
    T GetObjectById(schema::ObjectId id) const;

    template <typename T>
    const std::string& GetLabel(const T& object) const;

    const std::string& GetLabel(schema::ObjectId id) const;

  private:
    ReplayImpl(wgpu::Device device, std::unique_ptr<CaptureImpl> capture);

    std::unique_ptr<DawnRootCommandVisitor> mVisitor;
    ReplayImplBase mBase;
};

}  // namespace dawn::replay

#endif  // SRC_DAWN_REPLAY_REPLAYIMPL_H_
