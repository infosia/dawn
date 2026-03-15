package androidx.webgpu.helper

import androidx.webgpu.GPUTexture

public class GPUImportedRGBHardwareBuffer(
  private val handle: Long,
  public val texture: GPUTexture,
) : AutoCloseable {

  override fun close() {
    destroyNative()
  }

  public external fun beginAccessNative()
  public external fun endAccessNative()
  public external fun destroyNative()
}