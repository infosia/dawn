# Performance & Memory (Phase 8)

This page highlights Dawn’s key caches, invalidation triggers, resource lifetime patterns, and common hot paths. Each item includes concrete code anchors to speed up reviews.

## Caches and invalidation

- Device and pipeline caches
  - `Device::GetOrCreatePipelineCache(const CacheKey&)` – central entry for backend pipeline caches.
    - See `src/dawn/native/Device.h` (GetOrCreatePipelineCache, GetBlobCache, StoreCachedBlob, GetCacheKey).
  - `PipelineCacheBase` and backend variants:
    - Vulkan: `src/dawn/native/vulkan/PipelineCacheVk.h` (Create / CreateMonolithic / StoreOnIdle semantics).
    - D3D12: root signature blob contributes to cache keys: `src/dawn/native/d3d12/PipelineLayoutD3D12.h` (`mRootSignatureBlob`).
    - Metal: encodes state via `MTL*PipelineState`; compiles are stored via `GetBlobCache()->EnsureStored(...)` (e.g., `src/dawn/native/metal/ShaderModuleMTL.mm`).
- Cache keys & object caching
  - `CachedObject`, `CacheKey`, `stream::Stream<CacheKey>`: `src/dawn/native/CachedObject.h`, `src/dawn/native/CacheKey.{h,cpp}`.
  - Render/Compute pipelines stream device info into `mCacheKey`:
    - `src/dawn/native/RenderPipeline.cpp` (Initialize the cache key lines ~1077), Compute counterpart nearby.
- Content-less caches (immutable objects)
  - Content-less caches for BGL / PipelineLayout / Sampler on `DeviceBase`:
    - See `src/dawn/native/PipelineLayout.h` (inherits `CachedObject` + `ContentLessObjectCacheable`), and `docs/dawn/device_facilities.md#immutable-object-caches`.
- BlobCache & toggles affecting caching
  - `DeviceBase::GetBlobCache()` and toggles in `src/dawn/native/Toggles.cpp`:
    - `disable_blob_cache`, `use_monolithic_pipeline_cache_vulkan`, `blob_cache_hash_validation`.
- Invalidation & isolation
  - Isolation keys impact cache segmentation (see tests in `src/dawn/tests/end2end/PipelineCachingTests.cpp` and `ShaderModuleCachingTests.cpp`).
  - Pipeline mismatches (layout, shader hash, targets) bypass cache – see negative cases in the same tests.

## Resource lifetime, ref-counting, and sync boundaries

- Ref-counting
  - `RefCounted`, `RefCountedWithExternalCount`: `src/dawn/common/RefCounted.h` (base for many object handles).
  - Vulkan handle wrappers: `src/dawn/native/vulkan/RefCountedVkHandle.h`.
- Device lifecycle
  - State machine and destruction path: `docs/dawn/device_facilities.md#device-lifecycle`; `src/dawn/native/Device.h`.
  - Backend destruction hooks (e.g., D3D12 queue / fences): `src/dawn/native/d3d12/QueueD3D12.cpp` (reset & fence notes).
- Submit/use validation boundaries
  - Queue submit validation calls: `src/dawn/native/Queue.cpp` (`ValidateSubmit`, `ValidateOnSubmittedWorkDone`).
  - Per-object readiness checks: `ValidateCanUseInSubmitNow()` on `CommandBuffer`, buffers, textures, query sets.
- Recycling & deferred release
  - Descriptor set / BGL recycling (Vulkan) and release behavior – see comments in tests like `DestroyTests.cpp` and `BindGroupTests.cpp`.

## Common hot paths and mitigations

- Command encoding in tight loops
  - Prefer pre-building render bundles for repeated work; reuse `PipelineLayout` / BGLs to avoid thrash.
  - Ensure vertex formats and strides are valid to skip slow paths (see `RenderPipeline.cpp` validation blocks with `DAWN_INVALID_IF`).
- Shader / pipeline compilation
  - Cache shader modules and pipelines; avoid incidental differences in descriptors that defeat caching.
  - For Vulkan, consider monolithic cache toggle and ensure `PerformIdleTasks()` integration if used.
- Texture copies and alignment
  - Correct row pitch and alignment avoid extra copies; validation helpers enforce constraints (see `Queue.cpp` → `ValidateLinearTextureData`, `ValidateTextureCopyRange`).
- Synchronization overhead
  - Batch transitions / barriers (backend-specific); avoid calling `WaitForAllOperations()` in hot paths.

## Quick review checklist (perf & memory)

- Cache keys stable? Descriptor/state canonicalized? BlobCache enabled unless intentionally disabled.
- Reuse immutable objects (BGLs, PipelineLayout, Sampler) via device caches.
- No unnecessary validation in hot loops (use precomputation or bundles).
- Lifetime clear on destruction paths; no ref-cycles; backend-specific releases in place.
- Submit-time readiness checks satisfied (no surprise sync).

## References

- `src/dawn/native/Device.h`, `CacheKey.{h,cpp}`, `PipelineLayout.h`, `RenderPipeline.cpp`.
- Backends: `vulkan/PipelineCacheVk.h`, `d3d12/PipelineLayoutD3D12.h`, `metal/ShaderModuleMTL.mm`.
- Validation: `src/dawn/native/Queue.cpp` readiness + copy validators.
- Tests: `src/dawn/tests/end2end/PipelineCachingTests.cpp`, `ShaderModuleCachingTests.cpp`, `MultithreadTests.cpp` (caching under concurrency).
