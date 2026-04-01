package androidx.webgpu.helper

import android.os.ParcelFileDescriptor
import androidx.webgpu.GPUSyncFence

/**
 * Base abstract wrapper for imported hardware buffers.
 * Manages the native handle and synchronization fences for accessing the buffer.
 */
public abstract class GPUImportedHardwareBuffer(
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
   * Ends access to the hardware buffer, returning any signal fences from the native layer.
   * @return Array of GPUSyncFence objects that will be signaled when WebGPU is done with the buffer.
   */
  public fun endAccess(): Array<GPUSyncFence> {
    val parcelFileDescriptors = nativeEndAccess(handle)
    return parcelFileDescriptors.map { pfd -> GPUSyncFence(pfd) }.toTypedArray()
  }

  override fun close() {
    nativeDestroy(handle)
  }

  protected external fun nativeBeginAccess(handle: Long, fences: Array<ParcelFileDescriptor>?)
  protected external fun nativeEndAccess(handle: Long): Array<ParcelFileDescriptor>
  protected external fun nativeDestroy(handle: Long)
}