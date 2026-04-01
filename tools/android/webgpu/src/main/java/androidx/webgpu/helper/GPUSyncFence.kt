package androidx.webgpu.helper

import android.hardware.SyncFence
import android.os.Build
import android.os.ParcelFileDescriptor
import android.os.Parcel
import androidx.annotation.RequiresApi

/**
 * A unified abstraction for fence synchronization in WebGPU
 *
 * This class is backed by a `ParcelFileDescriptor` representing a sync fence.
 */
public class GPUSyncFence internal constructor(
    private val parcelFileDescriptor: ParcelFileDescriptor
) : AutoCloseable {

    /**
     * Waits for the fence to signal.
     */
    public fun await(timeoutMillis: Long): Boolean {
        return awaitParcelFileDescriptorNative(parcelFileDescriptor, timeoutMillis)
    }

    private external fun awaitParcelFileDescriptorNative(pfd: ParcelFileDescriptor, timeoutMillis: Long): Boolean

    /**
     * Extracts the ParcelFileDescriptor.
     * This is intended for internal library use or when passing to JNI.
     */
    internal fun getParcelFileDescriptor(): ParcelFileDescriptor {
        return parcelFileDescriptor
    }

    /**
     * Converts to platform SyncFence if on API 33+.
     * Uses GPUSyncFenceHelper to perform the conversion safely.
     */
    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    public fun toSyncFence(): SyncFence? {
        val duplicatedPfd = parcelFileDescriptor.dup()
        val rawFd = duplicatedPfd.detachFd()
        return GPUSyncFenceHelper.createSyncFenceFromFd(rawFd)
    }

    override fun close() {
        parcelFileDescriptor.close()
    }

    public companion object {
        /**
         * Creates a GPUSyncFence from a platform SyncFence by extracting its file descriptor.
         */
        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        public fun fromSyncFence(syncFence: SyncFence): GPUSyncFence {
            val pfd = readFileDescriptor(syncFence)
            return GPUSyncFence(pfd)
        }

        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        private fun readFileDescriptor(syncFence: SyncFence): ParcelFileDescriptor {
            val parcel = Parcel.obtain()
            try {
                syncFence.writeToParcel(parcel, 0)
                parcel.setDataPosition(0)
                check(parcel.readBoolean()) { "Failed to read boolean from parcel" }
                val pfd = parcel.readFileDescriptor()
                checkNotNull(pfd) { "Failed to read file descriptor from parcel" }
                return pfd
            } finally {
                parcel.recycle()
            }
        }
    }
}