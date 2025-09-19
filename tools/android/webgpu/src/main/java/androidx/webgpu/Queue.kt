package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Queue(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("onSubmittedWorkDone")
    public external fun onSubmittedWorkDone(callbackInfo: QueueWorkDoneCallbackInfo, ): Future

    /**
    * @param label    
    */
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    /**
    * @param commands    
    */
    @FastNative
    @JvmName("submit")
    public external fun submit(commands: Array<CommandBuffer> = arrayOf(), ): Unit

    /**
    * Produces a @ref DeviceError both content-timeline (`size` alignment) and device-
    * timeline errors defined by the WebGPU specification.
    * @param buffer
    * @param bufferOffset
    * @param data    
    */
    @FastNative
    @JvmName("writeBuffer")
    public external fun writeBuffer(buffer: Buffer, bufferOffset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Unit

    /**
    * @param destination
    * @param data
    * @param dataLayout
    * @param writeSize    
    */
    @FastNative
    @JvmName("writeTexture")
    public external fun writeTexture(destination: TexelCopyTextureInfo, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), dataLayout: TexelCopyBufferLayout, writeSize: Extent3D, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Queue && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
