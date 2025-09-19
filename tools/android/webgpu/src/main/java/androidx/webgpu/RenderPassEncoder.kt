package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class RenderPassEncoder(public val handle: Long): AutoCloseable {
    /**
    * @param queryIndex    
    */
    @FastNative
    @JvmName("beginOcclusionQuery")
    public external fun beginOcclusionQuery(queryIndex: Int, ): Unit

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

    @FastNative
    @JvmName("end")
    public external fun end(): Unit

    @FastNative
    @JvmName("endOcclusionQuery")
    public external fun endOcclusionQuery(): Unit

    /**
    * @param bundles    
    */
    @FastNative
    @JvmName("executeBundles")
    public external fun executeBundles(bundles: Array<RenderBundle> = arrayOf(), ): Unit

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
    * @param colorThe RGBA blend constant. Represents an `f32` color using @ref DoubleAsSupertype.    
    */
    @FastNative
    @JvmName("setBlendConstant")
    public external fun setBlendConstant(color: Color, ): Unit

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
    * @param x
    * @param y
    * @param width
    * @param height    
    */
    @FastNative
    @JvmName("setScissorRect")
    public external fun setScissorRect(x: Int, y: Int, width: Int, height: Int, ): Unit

    /**
    * @param reference    
    */
    @FastNative
    @JvmName("setStencilReference")
    public external fun setStencilReference(reference: Int, ): Unit

    /**
    * @param slot
    * @param buffer
    * @param offset
    * @param size    
    */
    @FastNative
    @JvmName("setVertexBuffer")
    public external fun setVertexBuffer(slot: Int, buffer: Buffer? = null, offset: Long = 0, size: Long = Constants.WHOLE_SIZE, ): Unit

    /**
    * If any argument is non-finite, produces a @ref NonFiniteFloatValueError.
    * @param x
    * @param y
    * @param width
    * @param height
    * @param minDepth
    * @param maxDepth    
    */
    @FastNative
    @JvmName("setViewport")
    public external fun setViewport(x: Float, y: Float, width: Float, height: Float, minDepth: Float, maxDepth: Float, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is RenderPassEncoder && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
