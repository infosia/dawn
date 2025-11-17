# API → Native → Backend interaction (sketch)

- Application calls WebGPU API (C/WebGPU C++ wrapper) → procs → dawn_proc forwards to proc table.
- dawn_native frontend:
  - Instance enumerates adapters → Adapter.CreateDevice → Device holds queues and caches.
  - Validation on creation and encode-time: descriptors, usages, limits, feature gating.
  - Encoding: CommandEncoder records commands → CommandBuffer finalized.
  - Queue.Submit: translate to backend command lists.
- Backend translation:
  - D3D12/Metal/Vulkan specialized objects wrap native handles.
  - Pipelines: ShaderModule (via Tint) → PipelineLayout → Render/ComputePipeline native creation.
  - Resources: Buffer/Texture/View create native resources; barriers managed per backend.
- Execution:
  - Queue submit enqueues native command buffers; fences/timeline handled per backend.
