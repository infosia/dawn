# Dawn Resolve Rect

The `dawn-resolve-rect` feature is an extension to `dawn-load-resolve-texture`, which in addition allows to specify a rect sub-region of texture, where load and resolve will take effect only. The feature can't be available unless `dawn-load-resolve-texture` is available.

The differences between `dawn-resolve-rect` feature and `dawn-partial-load-resolve-texture` feature: `dawn-resolve-rect` uses a render target of size specified by {`width`, `height`} of RenderPassDescriptorResolveRect, while `dawn-partial-load-resolve-texture` creates a full sized render target.

Additional functionalities:
 - Adds `wgpu::RenderPassDescriptorResolveRect` as chained struct for `wgpu::RenderPassDescriptor`. It defines a rect of {`colorOffsetX`, `colorOffsetY`, `width`, `height`} and {`resolveOffsetX`, `resolveOffsetY`, `width`, `height`} to indicate that expanding and resolving are only performed partially on the texels within this rect region of texture. The texels outside of the rect are not impacted.

Example Usage:
```
// In the first render pass, create a full-sized texture with TextureBinding usage, draw into it. This texture will be used as resolve target for all render passes.
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
desc.size.width = fullWidth;
desc.size.height = fullHeight;
auto resolveTexture = device.CreateTexture(&desc);

// Create MSAA texture
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment;
desc.sampleCount = 4;

auto msaaTexture = device.CreateTexture(&desc);

// Create a render pass which will discard the MSAA texture.
wgpu::RenderPassDescriptor renderPassDesc = ...;
renderPassDesc.colorAttachments[0].view = msaaTexture.CreateView();
renderPassDesc.colorAttachments[0].resolveTarget
 = resolveTexture.CreateView();
renderPassDesc.colorAttachments[0].storeOp
 = wgpu::StoreOp::Discard;

auto renderPassEncoder = encoder.BeginRenderPass(&renderPassDesc);
renderPassEncoder.Draw(3);
renderPassEncoder.End();

// Create a render pipeline with wgpu::ColorTargetStateExpandResolveTextureDawn.
wgpu::ColorTargetStateExpandResolveTextureDawn pipelineExpandResolveTextureState;
pipelineExpandResolveTextureState.enabled = true;

wgpu::RenderPipelineDescriptor pipelineDesc = ...;
pipelineDesc.multisample.count = 4;

pipelineDesc->fragment->targets[0].nextInChain = &pipelineExpandResolveTextureState;

auto pipeline = device.CreateRenderPipeline(&pipelineDesc);

// Create a partial MSAA texture as the "ExpandResolveTexture" target.
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment;
desc.size.width = partialWidth;
desc.size.height = partialHeight;
desc.sampleCount = 4;

auto partialMsaaTexture = device.CreateTexture(&desc);

// Create another render pass with "ExpandResolveTexture" LoadOp.
// Even though we discard the previous content of the MSAA texture,
// the old pixels of the resolve texture will be reserved across
// render passes.

wgpu::RenderPassDescriptor renderPassDesc2 = ...;
// Use the partial texture as MSAA target.
renderPassDesc2.colorAttachments[0].view = partialMsaaTexture.CreateView();
// Use the full sized texture as resolve target.
renderPassDesc2.colorAttachments[0].resolveTarget
 = resolveTexture.CreateView();
renderPassDesc2.colorAttachments[0].loadOp
 = wgpu::LoadOp::ExpandResolveTexture;

// If there is no need to expand and resolve the whole texture,
// RenderPassDescriptorResolveRect can be used to specify a
// subregion of texture to be updated only.
wgpu::RenderPassDescriptorResolveRect rect{};
rect.colorOffsetX = rect.colorOffsetY = 1;
rect.resolveOffsetX = rect.resolveOffsetY = 1;
rect.width = rect.height = 32;
renderPassDesc2.nextInChain = &rect;

auto renderPassEncoder2 = encoder.BeginRenderPass(&renderPassDesc2);
renderPassEncoder2.SetPipeline(pipeline);
renderPassEncoder2.Draw(3);
renderPassEncoder2.End();

```

Notes:
 - The feature currently is only available on dawn d3d11 backend. Internally, both expand and resolve are implemented with a dedicated `wgpu::RenderPipeline`. `wgpu::RenderPassEncoder::APISetScissorRect` is used to set the scissor rect to `wgpu::RenderPassDescriptorResolveRect`, when using the pipeline. The major difference is that expand lives in the original render pass, while resolve requires a separate one.