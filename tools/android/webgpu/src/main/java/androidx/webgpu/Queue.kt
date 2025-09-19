package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Queue(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("onSubmittedWorkDone")
    public external fun onSubmittedWorkDone(callbackInfo: QueueWorkDoneCallbackInfo, ): Future

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    @FastNative
    @JvmName("submit")
    public external fun submit(commands: Array<CommandBuffer> = arrayOf(), ): Unit

    /**
     * Produces a @see DeviceError both content-timeline (`size` alignment) and
     * device-timeline errors defined by the WebGPU specification.
     */
    @FastNative
    @JvmName("writeBuffer")
    public external fun writeBuffer(buffer: Buffer, bufferOffset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Unit

    @FastNative
    @JvmName("writeTexture")
    public external fun writeTexture(destination: TexelCopyTextureInfo, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), dataLayout: TexelCopyBufferLayout, writeSize: Extent3D, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Queue && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
