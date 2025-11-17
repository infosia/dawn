# Backend flows (annotated)

This ref focuses on concrete entry points and data paths per backend, with file/symbol pointers for quick jumps.

- Vulkan (src/dawn/native/vulkan)
  - Pipeline layout creation and caching:
    - `PipelineLayout::CreateVkPipelineLayout()` and `GetOrCreateVkLayoutObject()` in `PipelineLayoutVk.cpp` manage `VkPipelineLayout` creation and a per-immediate-constant-size cache via `mVkPipelineLayouts`.
    - `PipelineVk::Initialize()` in `PipelineVk.cpp` wires the layout (`PipelineVk::GetVkLayout`).
  - Submission:
    - `QueueVk::SubmitImpl()` wraps `vkQueueSubmit` with `TRACE_EVENT_*` in `QueueVk.cpp`.
  - Buffer usage and barriers:
    - `BufferVk::TrackUsageAndGetResourceBarrier(...)` and follow-on `EmitBufferBarriers()` in `BufferVk.cpp`; note comments on implicit domain / visibility around `vkQueueSubmit`.
  - Object lifetime and deletion:
    - `FencedDeleter` queues `VkPipeline`, `VkPipelineLayout`, `VkQueryPool` for deferred deletion.

- D3D12 (src/dawn/native/d3d12)
  - Root signatures and pipeline layouts:
    - `PipelineLayoutD3D12.{h,cpp}` builds a `ID3D12RootSignature` (serialize v1.0 from v1.1 when needed), stores `mRootSignatureBlob` for cache keys, and exposes `GetRootSignature()`.
    - Root signature binding is applied in `CommandBufferD3D12.cpp` (see `UpdateRootSignatureIfNecessary` and `SetRootSignature`).
  - Command recording and submit:
    - `CommandRecordingContext` wraps `ID3D12GraphicsCommandList*` and executes via `ExecuteCommandLists` in `CommandRecordingContext.cpp`.
    - Queue submit path via `QueueD3D12` (`GetCommandQueue()`, `GetPendingCommandContext()`).
  - Resource state tracking and barriers:
    - `Buffer::TrackUsageAndGetResourceBarrier()` and `TrackUsageAndTransitionNow()` in `BufferD3D12.cpp` (notes on implicit promotion/decay across `ExecuteCommandLists`).
    - `Texture::TransitionUsageAndGetResourceBarrier(...)` in `TextureD3D12.cpp` encodes `ResourceBarrier`s and calls `ID3D12GraphicsCommandList::ResourceBarrier`.

- Metal (src/dawn/native/metal)
  - Pipeline state creation:
    - `RenderPipelineMTL.mm`: `newRenderPipelineStateWithDescriptor:` → `mMtlRenderPipelineState` (with metrics instrumentation).
    - `ComputePipelineMTL.mm`: `newComputePipelineStateWithDescriptor` → `mMtlComputePipelineState`.
  - Encoding:
    - `RenderPipeline::Encode()` sets `setRenderPipelineState:`; `ComputePipeline::Encode()` sets `setComputePipelineState:`.
  - Buffer ops and state:
    - `BufferMTL.mm` tracks usage (`MarkUsedInPendingCommands`) and encodes blits / fills.

Backend-agnostic vs overrides
- Frontend validation and object models live in `src/dawn/native/*` (non-backend directories), while backends override translation and submission details.
- Cache keys (`CacheKey`, `CachedObject`) and pipeline caches (`PipelineCacheBase` and backend variants) are unified concepts with per-backend storage/serialization.

Cross-cutting caches
- `RenderPipeline.cpp` and `ComputePipeline.cpp` stream device info into `mCacheKey`.
- Vulkan uses `RefCountedVkHandle` wrappers for shared handles; D3D12 caches root signature blobs; Metal caches `MTL*PipelineState` objects.
