# MSAA Render To Single Sampled/Multisampled Render To Single Sampled

The `msaa-render-to-single-sampled` feature allows a render pass to include single-sampled attachments while rendering is done with a specified number of samples. When this feature is used, the client doesn't need to explicitly allocate any multi-sampled color textures. We denote this kind of render passes as "implicit multi-sampled" render passes.

Additional functionalities:
 - Adds `wgpu::DawnRenderPassSampleCount` as chained struct for `wgpu::RenderPassDescriptor` to specify number of samples to be rendered for the respective single-sampled attachment.

Example Usage:
```
// Create texture with RenderAttachment usage.
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment;

auto texture = device.CreateTexture(&desc);

// Create a render pipeline
wgpu::RenderPipelineDescriptor pipelineDesc = ...;
auto pipeline = device.CreateRenderPipeline(&pipelineDesc);

// Create a render pass with "implicit multi-sampled" enabled.
wgpu::DawnRenderPassSampleCount renderPassSampleCount;
renderPassSampleCount.sampleCount = 4;

wgpu::RenderPassDescriptor renderPassDesc = ...;
renderPassDesc.colorAttachments[0].view = texture.CreateView();
renderPassDesc.nextInChain = &renderPassSampleCount;

auto renderPassEncoder = encoder.BeginRenderPass(&renderPassDesc);

renderPassEncoder.SetPipeline(pipeline);
renderPassEncoder.Draw(3);
renderPassEncoder.End();
```

Notes:
 - If a texture is attached to a "implicit multi-sampled" render pass, it must be single-sampled. It mustn't be assigned to the `resolveTarget` field of the the render pass' color attachment.
 - Depth stencil textures can be attached to a "implicit multi-sampled" render pass. But its sample count must match the number specified in one color attachment's `wgpu::DawnRenderPassSampleCount`'s `sampleCount` field.
 - Currently all color attachments will be implicitly multi-sampled when the render pass has the feature enabled, this could be changed in future.
 - The texture is not supported if it is not resolvable by WebGPU standard. This means this feature currently doesn't work with integer textures.
