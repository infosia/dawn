package androidx.webgpu.helper

/**
 * Base abstract wrapper for imported hardware buffers.
 */
public abstract class GPUImportedHardwareBuffer(
  private val handle: Long
) : AutoCloseable {

  public fun beginAccess() { nativeBeginAccess(handle) }
  public fun endAccess() { nativeEndAccess(handle) }

  override fun close() {
    nativeDestroy(handle)
  }

  protected external fun nativeBeginAccess(handle: Long)
  protected external fun nativeEndAccess(handle: Long)
  protected external fun nativeDestroy(handle: Long)
}