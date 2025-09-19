package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class CommandEncoder(public val handle: Long): AutoCloseable {
    /**
    * @param descriptor    
    */
    @FastNative
    @JvmName("beginComputePass")
    public external fun beginComputePass(descriptor: ComputePassDescriptor? = ComputePassDescriptor(), ): ComputePassEncoder

    /**
    * @param descriptor    
    */
    @FastNative
    @JvmName("beginRenderPass")
    public external fun beginRenderPass(descriptor: RenderPassDescriptor, ): RenderPassEncoder

    /**
    * @param buffer
    * @param offset
    * @param size    
    */
    @FastNative
    @JvmName("clearBuffer")
    public external fun clearBuffer(buffer: Buffer, offset: Long = 0, size: Long = Constants.WHOLE_SIZE, ): Unit

    /**
    * @param source
    * @param sourceOffset
    * @param destination
    * @param destinationOffset
    * @param size    
    */
    @FastNative
    @JvmName("copyBufferToBuffer")
    public external fun copyBufferToBuffer(source: Buffer, sourceOffset: Long, destination: Buffer, destinationOffset: Long, size: Long, ): Unit

    /**
    * @param source
    * @param destination
    * @param copySize    
    */
    @FastNative
    @JvmName("copyBufferToTexture")
    public external fun copyBufferToTexture(source: TexelCopyBufferInfo, destination: TexelCopyTextureInfo, copySize: Extent3D, ): Unit

    /**
    * @param source
    * @param destination
    * @param copySize    
    */
    @FastNative
    @JvmName("copyTextureToBuffer")
    public external fun copyTextureToBuffer(source: TexelCopyTextureInfo, destination: TexelCopyBufferInfo, copySize: Extent3D, ): Unit

    /**
    * @param source
    * @param destination
    * @param copySize    
    */
    @FastNative
    @JvmName("copyTextureToTexture")
    public external fun copyTextureToTexture(source: TexelCopyTextureInfo, destination: TexelCopyTextureInfo, copySize: Extent3D, ): Unit

    /**
    * @param descriptor    
    */
    @FastNative
    @JvmName("finish")
    public external fun finish(descriptor: CommandBufferDescriptor? = CommandBufferDescriptor(), ): CommandBuffer

    /**
    * @param markerLabel    
    */
    @FastNative
    @JvmName("insertDebugMarker")
    public external fun insertDebugMarker(markerLabel: String, ): Unit

    @FastNative
    @JvmName("popDebugGroup")
    public external fun popDebugGroup(): Unit

    /**
    * @param groupLabel    
    */
    @FastNative
    @JvmName("pushDebugGroup")
    public external fun pushDebugGroup(groupLabel: String, ): Unit

    /**
    * @param querySet
    * @param firstQuery
    * @param queryCount
    * @param destination
    * @param destinationOffset    
    */
    @FastNative
    @JvmName("resolveQuerySet")
    public external fun resolveQuerySet(querySet: QuerySet, firstQuery: Int, queryCount: Int, destination: Buffer, destinationOffset: Long, ): Unit

    /**
    * @param label    
    */
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    /**
    * @param querySet
    * @param queryIndex    
    */
    @FastNative
    @JvmName("writeTimestamp")
    public external fun writeTimestamp(querySet: QuerySet, queryIndex: Int, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is CommandEncoder && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
