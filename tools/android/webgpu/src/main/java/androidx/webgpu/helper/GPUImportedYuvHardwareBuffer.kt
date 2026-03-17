package androidx.webgpu.helper

import androidx.webgpu.GPUExternalTexture

public class GPUImportedYuvHardwareBuffer(
  private val handle: Long,
  public val externalTexture: GPUExternalTexture,
) : AutoCloseable {

  public external fun beginAccessNative()
  public external fun endAccessNative()
  private external fun destroyNative()

  override fun close() {
    destroyNative()
  }
}