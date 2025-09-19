package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class RenderBundleEncoder(public val handle: Long): AutoCloseable {
    /**
    * @param vertexCount
    * @param instanceCount
    * @param firstVertex
    * @param firstInstance    
    */
    @FastNative
    @JvmName("draw")
    public external fun draw(vertexCount: Int, instanceCount: Int = 1, firstVertex: Int = 0, firstInstance: Int = 0, ): Unit

    /**
    * @param indexCount
    * @param instanceCount
    * @param firstIndex
    * @param baseVertex
    * @param firstInstance    
    */
    @FastNative
    @JvmName("drawIndexed")
    public external fun drawIndexed(indexCount: Int, instanceCount: Int = 1, firstIndex: Int = 0, baseVertex: Int = 0, firstInstance: Int = 0, ): Unit

    /**
    * @param indirectBuffer
    * @param indirectOffset    
    */
    @FastNative
    @JvmName("drawIndexedIndirect")
    public external fun drawIndexedIndirect(indirectBuffer: Buffer, indirectOffset: Long, ): Unit

    /**
    * @param indirectBuffer
    * @param indirectOffset    
    */
    @FastNative
    @JvmName("drawIndirect")
    public external fun drawIndirect(indirectBuffer: Buffer, indirectOffset: Long, ): Unit

    /**
    * @param descriptor    
    */
    @FastNative
    @JvmName("finish")
    public external fun finish(descriptor: RenderBundleDescriptor? = RenderBundleDescriptor(), ): RenderBundle

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
    * @param groupIndex
    * @param group
    * @param dynamicOffsets    
    */
    @FastNative
    @JvmName("setBindGroup")
    public external fun setBindGroup(groupIndex: Int, group: BindGroup? = null, dynamicOffsets: IntArray = intArrayOf(), ): Unit

    /**
    * @param buffer
    * @param format
    * @param offset
    * @param size    
    */
    @FastNative
    @JvmName("setIndexBuffer")
    public external fun setIndexBuffer(buffer: Buffer, format: IndexFormat = IndexFormat.Undefined, offset: Long = 0, size: Long = Constants.WHOLE_SIZE, ): Unit

    /**
    * @param label    
    */
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    /**
    * @param pipeline    
    */
    @FastNative
    @JvmName("setPipeline")
    public external fun setPipeline(pipeline: RenderPipeline, ): Unit

    /**
    * @param slot
    * @param buffer
    * @param offset
    * @param size    
    */
    @FastNative
    @JvmName("setVertexBuffer")
    public external fun setVertexBuffer(slot: Int, buffer: Buffer? = null, offset: Long = 0, size: Long = Constants.WHOLE_SIZE, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is RenderBundleEncoder && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
