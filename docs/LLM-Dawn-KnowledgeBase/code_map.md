# Dawn core code map

- src/dawn/native
  - Frontend: Instance, Adapter, Device, Queue, CommandEncoder, CommandBuffer
  - Resources: Buffer, Texture, TextureView, Sampler, BindGroup, BindGroupLayout
  - Pipelines: ShaderModule, PipelineLayout, RenderPipeline, ComputePipeline
  - Validation: descriptor checks, usage tracking, error scopes
  - Backends: d3d12/, metal/, vulkan/
- src/dawn/wire
  - Client/server RPC; minimal client state; mock/test hooks
- src/dawn/common
  - Base utilities, ref-counting, optionals, bitsets, logging/assert helpers
- src/dawn/utils
  - Test helpers and sample scaffolding (not for production libraries)
- src/dawn/tests
  - unittests, end2end, perf_tests, white_box
