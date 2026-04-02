package androidx.webgpu.helper

import androidx.webgpu.GPUTexture

public class GPUHardwareBufferTexture(
    handle: Long,
    public val texture: GPUTexture,
) : GPUHardwareBufferWrapper(handle)