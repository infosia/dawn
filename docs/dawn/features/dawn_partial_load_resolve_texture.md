# Dawn partial Load Resolve Texture

The `dawn-partial-load-resolve-texture` feature is an extension to `dawn-load-resolve-texture`, which in addition allows to specify a rect sub-region of texture, where load and resolve will take effect only. The feature can't be available unless `dawn-load-resolve-texture` is available.

Additional functionalities:
 - Adds `wgpu::RenderPassDescriptorExpandResolveRect` as chained struct for `wgpu::RenderPassDescriptor`. It defines a rect of {`x`, `y`, `width`, `height`} to indicate that expanding and resolving are only performed partially on the texels within this rect region of texture. The texels outside of the rect are not impacted.

Example Usage:
```
// Create MSAA texture
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment;
// TextureBinding is extra required in case of RenderPassDescriptorExpandResolveRect.
desc.usage |= wgpu::TextureUsage::TextureBinding;
desc.sampleCount = 4;

auto msaaTexture = device.CreateTexture(&desc);


// Create resolve texture with TextureBinding usage.
wgpu::TextureDescriptor desc = ...;
desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

auto resolveTexture = device.CreateTexture(&desc);

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

// Create another render pass with "ExpandResolveTexture" LoadOp.
// Even though we discard the previous content of the MSAA texture,
// the old pixels of the resolve texture will be reserved across
// render passes.
wgpu::RenderPassDescriptor renderPassDesc2 = ...;
renderPassDesc2.colorAttachments[0].view = msaaTexture.CreateView();
renderPassDesc2.colorAttachments[0].resolveTarget
 = resolveTexture.CreateView();
renderPassDesc2.colorAttachments[0].loadOp
 = wgpu::LoadOp::ExpandResolveTexture;

// If there is no need to expand and resolve the whole texture,
// RenderPassDescriptorExpandResolveRect can be used to specify a
// subregion of texture to be updated only.
wgpu::RenderPassDescriptorExpandResolveRect rect{};
rect.x = rect.y = 0;
rect.width = rect.height = 32;
renderPassDesc2.nextInChain = &rect;

auto renderPassEncoder2 = encoder.BeginRenderPass(&renderPassDesc2);
renderPassEncoder2.SetPipeline(pipeline);
renderPassEncoder2.Draw(3);
renderPassEncoder2.End();

```

Notes:
 - If a view of multisampled texture is used in a `wgpu::LoadOp::ExpandResolveTexture` operation, and there is `wgpu::RenderPassDescriptorExpandResolveRect` chained in `wgpu::RenderPassDescriptor`, it must have `wgpu::TextureUsage::TextureBinding` usage.
