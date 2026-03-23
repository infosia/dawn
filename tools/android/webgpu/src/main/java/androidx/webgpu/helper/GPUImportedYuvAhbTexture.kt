package androidx.webgpu.helper

import androidx.webgpu.GPUExternalTexture

public class GPUImportedYuvAhbTexture(
  handle: Long,
  public val externalTexture: GPUExternalTexture,
) : GPUImportedHardwareBuffer(handle)
