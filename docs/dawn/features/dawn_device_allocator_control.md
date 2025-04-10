# Dawn Device Allocator Control (experimental)

This feature allows `wgpu::DawnDeviceAllocatorControl` to be chained on `wgpu::DeviceDescriptor`.
The feature will be enabled if it's supported. If `wgpu::DawnDeviceAllocatorControl` is chained and
the feature is not enabled then creating a device will fail with a validation error.

It is available in the Vulkan backend.
