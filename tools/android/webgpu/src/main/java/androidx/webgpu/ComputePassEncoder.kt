package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class ComputePassEncoder(public val handle: Long): AutoCloseable {
    /**
    * @param workgroupCountX
    * @param workgroupCountY
    * @param workgroupCountZ    
    */
    @FastNative
    @JvmName("dispatchWorkgroups")
    public external fun dispatchWorkgroups(workgroupCountX: Int, workgroupCountY: Int = 1, workgroupCountZ: Int = 1, ): Unit

    /**
    * @param indirectBuffer
    * @param indirectOffset    
    */
    @FastNative
    @JvmName("dispatchWorkgroupsIndirect")
    public external fun dispatchWorkgroupsIndirect(indirectBuffer: Buffer, indirectOffset: Long, ): Unit

    @FastNative
    @JvmName("end")
    public external fun end(): Unit

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
    public external fun setPipeline(pipeline: ComputePipeline, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is ComputePassEncoder && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
