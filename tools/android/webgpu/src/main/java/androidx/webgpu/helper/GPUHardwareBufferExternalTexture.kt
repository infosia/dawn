package androidx.webgpu.helper

import androidx.webgpu.GPUExternalTexture

public class GPUHardwareBufferExternalTexture(
  handle: Long,
  public val externalTexture: GPUExternalTexture,
) : GPUHardwareBufferWrapper(handle)
