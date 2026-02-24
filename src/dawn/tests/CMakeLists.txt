# Copyright 2026 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

dawn_add_library(
  dawn_test_common
  FORCE_STATIC
  UTILITY_TARGET dawn_internal_config
  HEADERS
    "AdapterTestConfig.h"
    "DawnTest.h"
    "ToggleParser.h"
    "PartitionAllocSupport.h"
    "mocks/platform/CachingInterfaceMock.cpp"
  SOURCES
    "AdapterTestConfig.cpp"
    "DawnTest.cpp"
    "PartitionAllocSupport.cpp"
    "ToggleParser.cpp"
    "mocks/platform/CachingInterfaceMock.h"
  DEPENDS
    dawn_native
    dawn_wgpu_utils
    dawncpp
    gmock
)

add_executable(dawn_end2end_tests
    "end2end/AdapterCreationTests.cpp"
    "end2end/AdapterEnumerationTests.cpp"
    "end2end/AdapterFormatCapabilitiesVkTests.cpp"
    "end2end/AdapterPropertiesD3DTests.cpp"
    "end2end/AdapterPropertiesVkTests.cpp"
    "end2end/ArchTierLimitsExhaustive.cpp"
    "end2end/BasicTests.cpp"
    "end2end/BindGroupTests.cpp"
    "end2end/BindingArrayTests.cpp"
    "end2end/BufferHostMappedPointerTests.cpp"
    "end2end/BufferHostMappedPointerTests.h"
    "end2end/BufferTests.cpp"
    "end2end/BufferZeroInitTests.cpp"
    "end2end/ClipDistancesTests.cpp"
    "end2end/ClipSpaceTests.cpp"
    "end2end/ColorStateTests.cpp"
    "end2end/CommandEncoderTests.cpp"
    "end2end/CompressedTextureFormatTests.cpp"
    "end2end/ComputeCopyStorageBufferTests.cpp"
    "end2end/ComputeDispatchTests.cpp"
    "end2end/ComputeFlowControlTests.cpp"
    "end2end/ComputeLayoutMemoryBufferTests.cpp"
    "end2end/ComputeSharedMemoryTests.cpp"
    "end2end/ComputeStorageBufferBarrierTests.cpp"
    "end2end/CopyExternalTextureForBrowserTests.cpp"
    "end2end/CopyTests.cpp"
    "end2end/CopyTextureForBrowserTests.cpp"
    "end2end/CreateDestroyTests.cpp"
    "end2end/CreatePipelineAsyncTests.cpp"
    "end2end/CullingTests.cpp"
    "end2end/DebugMarkerTests.cpp"
    "end2end/DepthBiasTests.cpp"
    "end2end/DepthStencilCopyTests.cpp"
    "end2end/DepthStencilLoadOpTests.cpp"
    "end2end/DepthStencilSamplingTests.cpp"
    "end2end/DepthStencilStateTests.cpp"
    "end2end/DestroyTests.cpp"
    "end2end/DeviceInitializationTests.cpp"
    "end2end/DeviceLifetimeTests.cpp"
    "end2end/DeviceLostTests.cpp"
    "end2end/DiscardBasicTests.cpp"
    "end2end/DrawIndexedIndirectTests.cpp"
    "end2end/DrawIndexedTests.cpp"
    "end2end/DrawIndirectTests.cpp"
    "end2end/DrawTests.cpp"
    "end2end/DualSourceBlendTests.cpp"
    "end2end/DynamicBufferOffsetTests.cpp"
    "end2end/EntryPointTests.cpp"
    "end2end/EventTests.cpp"
    "end2end/ExternalTextureTests.cpp"
    "end2end/FirstIndexOffsetTests.cpp"
    "end2end/FragDepthTests.cpp"
    "end2end/FramebufferFetchTests.cpp"
    "end2end/GpuMemorySynchronizationTests.cpp"
    "end2end/HistogramTests.cpp"
    "end2end/ImmediateDataBufferLengthTests.cpp"
    "end2end/ImmediateDataTests.cpp"
    "end2end/IndexFormatTests.cpp"
    "end2end/InfiniteLoopTests.cpp"
    "end2end/MaxLimitTests.cpp"
    "end2end/MemoryAllocationStressTests.cpp"
    "end2end/MemoryHeapPropertiesTests.cpp"
    "end2end/MultiDrawIndexedIndirectTests.cpp"
    "end2end/MultiDrawIndirectTests.cpp"
    "end2end/MultisampledInterpolationTests.cpp"
    "end2end/MultisampledRenderingTests.cpp"
    "end2end/MultisampledSamplingTests.cpp"
    "end2end/MultithreadTests.cpp"
    "end2end/NonzeroBufferCreationTests.cpp"
    "end2end/NonzeroTextureCreationTests.cpp"
    "end2end/OpArrayLengthTests.cpp"
    "end2end/PackUnpack4x8NormTests.cpp"
    "end2end/Packed4x8IntegerDotProductTests.cpp"
    "end2end/PipelineCachingTests.cpp"
    "end2end/PipelineLayoutTests.cpp"
    "end2end/PixelLocalStorageTests.cpp"
    "end2end/PolyfillBuiltinSimpleTests.cpp"
    "end2end/PrimitiveIndexTests.cpp"
    "end2end/PrimitiveStateTests.cpp"
    "end2end/PrimitiveTopologyTests.cpp"
    "end2end/QueryTests.cpp"
    "end2end/QueueTests.cpp"
    "end2end/QueueTimelineTests.cpp"
    "end2end/ReadOnlyDepthStencilAttachmentTests.cpp"
    "end2end/RenderAttachmentTests.cpp"
    "end2end/RenderBundleTests.cpp"
    "end2end/RenderPassLoadOpTests.cpp"
    "end2end/RenderPassTests.cpp"
    "end2end/RequiredBufferSizeInCopyTests.cpp"
    "end2end/ResourceTableTests.cpp"
    "end2end/SamplerFilterAnisotropicTests.cpp"
    "end2end/SamplerTests.cpp"
    "end2end/ScissorTests.cpp"
    "end2end/SelectNonShortCircuiting.cpp"
    "end2end/ShaderAtomicTests.cpp"
    "end2end/ShaderBuiltinPartialConstArgsErrorTests.cpp"
    "end2end/ShaderF16Tests.cpp"
    "end2end/ShaderModuleCachingTests.cpp"
    "end2end/ShaderPrintTests.cpp"
    "end2end/ShaderTests.cpp"
    "end2end/ShaderValidationTests.cpp"
    "end2end/StorageTextureTests.cpp"
    "end2end/SubgroupMatrixTests.cpp"
    "end2end/SubgroupsTests.cpp"
    "end2end/SubresourceRenderAttachmentTests.cpp"
    "end2end/SupportedFeatureArch.cpp"
    "end2end/Texture3DTests.cpp"
    "end2end/TextureComponentSwizzleTests.cpp"
    "end2end/TextureCorruptionTests.cpp"
    "end2end/TextureFormatTests.cpp"
    "end2end/TextureFormatsTier1Tests.cpp"
    "end2end/TextureShaderBuiltinTests.cpp"
    "end2end/TextureSubresourceTests.cpp"
    "end2end/TextureViewTests.cpp"
    "end2end/TextureZeroInitTests.cpp"
    "end2end/VertexFormatTests.cpp"
    "end2end/VertexOnlyRenderPipelineTests.cpp"
    "end2end/VertexStateTests.cpp"
    "end2end/ViewportOrientationTests.cpp"
    "end2end/ViewportTests.cpp"

    "End2EndTestsMain.cpp"
)
common_compile_options(dawn_end2end_tests)

set(conditional_private_platform_depends)
if (DAWN_ENABLE_D3D11 OR DAWN_ENABLE_D3D12)
  list(APPEND conditional_private_platform_depends dxgi.lib)
endif()

target_link_libraries(dawn_end2end_tests PRIVATE
  absl_strings
  dawn_common
  dawn_internal_config
  dawn_native
  dawn_native_objects
  dawn_platform
  dawn_proc
  dawn_shared_utils_chromium_test_compat
  dawn_system_utils
  dawn_test_common
  dawn_test_utils
  dawn_wgpu_utils
  dawncpp
  gmock

  ${conditional_private_platform_depends}
)
