# Dawn Load Store Op DontCare
The `dawn-load-store-op-dontcare` feature adds new options to `wgpu::LoadOp` and `wgpu::StoreOp`:
- `wgpu::LoadOp::DontCare`
  - This will be either Load or Clear depending on what driver sees as the best performining option.
- `wgpu::StoreOp::DontCare`
  - This will be either Store or Discard depending on what driver sees as the best performining option.
