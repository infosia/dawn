package {{ kotlin_package }}

import android.os.ParcelFileDescriptor

/**
 * Base abstract wrapper for imported hardware buffers.
 * Manages the native handle and synchronization fences for accessing the buffer.
 */
public abstract class GPUHardwareBufferWrapper(
  private val handle: Long
) : AutoCloseable {

  /**
   * Begins access to the hardware buffer, passing any wait fences to the native layer.
   * @param fences Array of GPUSyncFence objects to wait on before accessing the buffer.
   */
  public fun beginAccess(fences: Array<GPUSyncFence>?) {
    val pfds = fences?.mapNotNull { it.getParcelFileDescriptor() }?.toTypedArray()
    nativeBeginAccess(handle, pfds)
  }

  /**
   * Ends access to the hardware buffer, returning a signal fence from the native layer.
   * @return GPUSyncFence that will be signaled when WebGPU is done with the buffer.
   */
  public fun endAccess(): GPUSyncFence? {
    val pfd = nativeEndAccess(handle)
    return pfd?.let { GPUSyncFence(it) }
  }

  override fun close() {
    nativeDestroy(handle)
  }

  protected external fun nativeBeginAccess(handle: Long, fences: Array<ParcelFileDescriptor>?)
  protected external fun nativeEndAccess(handle: Long): ParcelFileDescriptor?
  protected external fun nativeDestroy(handle: Long)
}