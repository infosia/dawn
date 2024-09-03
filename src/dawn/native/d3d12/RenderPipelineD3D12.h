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

#ifndef SRC_DAWN_NATIVE_D3D12_RENDERPIPELINED3D12_H_
#define SRC_DAWN_NATIVE_D3D12_RENDERPIPELINED3D12_H_

#include "dawn/native/RenderPipeline.h"

#include "dawn/native/d3d12/ShaderModuleD3D12.h"
#include "dawn/native/d3d12/d3d12_platform.h"

namespace dawn::native::d3d12 {

class Device;

// Define here for quick prototyping. Should lives in a seperate header file.
class ImmediateDataManagerD3D12 : public ImmediateDataManagerBase {
  public:
    ImmediateDataManagerD3D12();
    void InitializeImmediateDataLayout(const ImmediateDataMetadataTable& metadatas);
    void Apply(ID3D12GraphicsCommandList* commandList, bool inCompute);
    uint32_t GetDynamicStorageBufferSizesBytes() const { return mDynamicStorageBufferSizesBytes; }
    uint32_t GetImmediateDataRegisterSpace() const { return mImmediateDataRegisterSpace; }
    uint32_t GetImmediateDataRegister() const { return mImmediateDataRegister; }
    uint32_t GetDynamicStorageBufferRegisterSpace() const {
        return mDynamicStorageBufferRegisterSpace;
    }
    uint32_t GetDynamicStorageBufferRegister() const { return mDynamicStorageBufferRegister; }

    void SetImmediateDataRootSignatureIndex(uint32_t index) {
        mImmediateDataRootSignatureIndex = index;
    }
    uint32_t GetImmediateDataRootSignatureIndex() const { return mImmediateDataRootSignatureIndex; }

    void SetDynamicStorageSizeRootSignatureIndex(uint32_t index) {
        mDynamicStorageSizeRootSignatureIndex = index;
    }
    uint32_t GetDynamicStorageSizeRootSignatureIndex() const {
        return mDynamicStorageSizeRootSignatureIndex;
    }

    // We don't need this after implementing unify immediate data block
    void UpdateDynamicStorageBufferSizeValue(ImmediateDataUsage usage,
                                             uint32_t offset,
                                             uint32_t size,
                                             const uint32_t* immediateData);
    // We don't need this after implementing unify immediate data block.
    void UpdateDynamicStorageBufferSizeMetadata(uint32_t rangeSize);

    bool HasImmediateDataMetaData(ImmediateDataUsage usage);
    const ImmediateDataMetadata& GetImmediateDataMetadata(ImmediateDataUsage usage);

  private:
    // ImmediateData shares the same range between shader stages.
    // TODO: Adding per stage immediate data support.
    uint32_t mDynamicStorageBufferSizesBytes = 0;

    // Temp range for dynamic storage buffer size, which uses seperated register
    std::array<uint32_t, kMaxImmediateDataCounts> mDynamicStorageBuffers;

    // D3D12 specific immediate data register info.
    uint32_t mImmediateDataRegisterSpace;
    uint32_t mImmediateDataRegister;

    // Temp register info for dynamic storage buffer size
    uint32_t mDynamicStorageBufferRegisterSpace;
    uint32_t mDynamicStorageBufferRegister;

    uint32_t mImmediateDataRootSignatureIndex;
    uint32_t mDynamicStorageSizeRootSignatureIndex;
};

class RenderPipeline final : public RenderPipelineBase {
  public:
    static Ref<RenderPipeline> CreateUninitialized(
        Device* device,
        const UnpackedPtr<RenderPipelineDescriptor>& descriptor);
    RenderPipeline() = delete;

    MaybeError InitializeImpl() override;

    D3D12_PRIMITIVE_TOPOLOGY GetD3D12PrimitiveTopology() const;
    ID3D12PipelineState* GetPipelineState() const;

    bool UsesVertexOrInstanceIndex() const;

    // Dawn API
    void SetLabelImpl() override;

    ComPtr<ID3D12CommandSignature> GetDrawIndirectCommandSignature();

    ComPtr<ID3D12CommandSignature> GetDrawIndexedIndirectCommandSignature();
    ImmediateDataManagerD3D12* GetImmediateDataManager();

  private:
    ~RenderPipeline() override;

    void DestroyImpl() override;

    ID3D12CommandSignature* GetDrawIndirectCommandSignatureWithInstanceVertexOffsets();

    ID3D12CommandSignature* GetDrawIndexedIndirectCommandSignatureWithInstanceVertexOffsets();

    using RenderPipelineBase::RenderPipelineBase;
    D3D12_INPUT_LAYOUT_DESC ComputeInputLayout(
        std::array<D3D12_INPUT_ELEMENT_DESC, kMaxVertexAttributes>* inputElementDescriptors);
    D3D12_DEPTH_STENCIL_DESC ComputeDepthStencilDesc();

    D3D12_PRIMITIVE_TOPOLOGY mD3d12PrimitiveTopology;
    ComPtr<ID3D12PipelineState> mPipelineState;
    bool mUsesVertexOrInstanceIndex;

    std::unique_ptr<ImmediateDataManagerD3D12> mImmediateDataManager;
};

}  // namespace dawn::native::d3d12

#endif  // SRC_DAWN_NATIVE_D3D12_RENDERPIPELINED3D12_H_
