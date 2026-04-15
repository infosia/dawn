package {{ kotlin_package }}

public class GPUHardwareBufferExternalTexture(
  handle: Long,
  public val externalTexture: GPUExternalTexture,
) : GPUHardwareBufferWrapper(handle)
