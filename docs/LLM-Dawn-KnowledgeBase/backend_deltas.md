# Backend delta table (with pointers)

| Area | Vulkan | D3D12 | Metal |
|---|---|---|---|
| Pipeline layout | `PipelineLayoutVk.cpp` creates/caches `VkPipelineLayout` | `PipelineLayoutD3D12.{h,cpp}` builds `ID3D12RootSignature` (+ blob for cache key) | Encoded in `RenderPipelineMTL.mm`/`ComputePipelineMTL.mm` via descriptors |
| Pipeline creation | `PipelineVk` ties to `mVkPipelineLayout` | PSO built with `pRootSignature = GetRootSignature()`; see `ComputePipelineD3D12.cpp` | `newRenderPipelineStateWithDescriptor:` / `newComputePipelineStateWithDescriptor:` |
| Bindings | Descriptor sets (set layouts) | Descriptor heaps + root descriptors/tables; see `CommandBufferD3D12.cpp` root binding application | Encoder resource slots; argument buffers (`MultiDrawEncoder`) |
| Barriers & states | Explicit Vulkan barriers | `ResourceBarrier` calls with implicit promotion/decay semantics (see `BufferD3D12.cpp`, `TextureD3D12.cpp`) | Mostly implicit state with encoders; explicit when needed via blit/encode setup |
| Memory | Backend device alloc + fenced deleter (`FencedDeleter`) | `ID3D12Resource` on heaps, residency mgmt; fenced waits in queue | Metal manages resource memory; Dawn tracks usage/ref |
| Queries/timestamps | `VkQueryPool` + `vkGetQueryPoolResults` | D3D12 timestamp frequency and calibration via `GetTimestampFrequency` / `GetClockCalibration` | Platform-limited support |
| Shader format | SPIR-V | HLSL/DXIL | MSL |
| Cache keys | Streamed via `CacheKey` + device info | Includes `mRootSignatureBlob` | Pipeline descriptors capture state; object caches per device |

Notes
- See `PipelineCacheBase` for unified cache behaviors; `BlobCache` for serialization.
- D3D12 has extra handling for immediate data and internal constants; Vulkan uses push constants through immediate masks.
