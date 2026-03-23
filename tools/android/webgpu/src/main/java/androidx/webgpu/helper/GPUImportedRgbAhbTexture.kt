package androidx.webgpu.helper

import androidx.webgpu.GPUTexture

public class GPUImportedRgbAhbTexture(
    handle: Long,
    public val texture: GPUTexture,
) : GPUImportedHardwareBuffer(handle)