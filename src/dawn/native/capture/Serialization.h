// Copyright 2025 The Dawn & Tint Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_NATIVE_CAPTURE_SERIALIZATION_H_
#define SRC_DAWN_NATIVE_CAPTURE_SERIALIZATION_H_

#include <cstdint>

namespace dawn::native::capture {

// A magic number to identify capture files.
// "DCAP" followed by a version number.
constexpr uint8_t kMagicNumber[] = {'D', 'C', 'A', 'P'};
constexpr uint32_t kVersionNumber = 1u;

enum class ObjectType : uint32_t {
    Device,
    Buffer,
    Texture,
    TextureView,
    Sampler,
    BindGroupLayout,
    PipelineLayout,
    ShaderModule,
    RenderPipeline,
    ComputePipeline,
    BindGroup,
    CommandBuffer,
    QuerySet,
};

enum class CaptureCommand : uint32_t {
    QueueSubmit,
    WriteBuffer,
    WriteTexture,
    Unmap,
};

}  // namespace dawn::native::capture

#endif  // SRC_DAWN_NATIVE_CAPTURE_SERIALIZATION_H_
