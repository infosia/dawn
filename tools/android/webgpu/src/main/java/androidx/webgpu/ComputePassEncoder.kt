package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class ComputePassEncoder(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("dispatchWorkgroups")
    public external fun dispatchWorkgroups(workgroupCountX: Int, workgroupCountY: Int = 1, workgroupCountZ: Int = 1, ): Unit

    @FastNative
    @JvmName("dispatchWorkgroupsIndirect")
    public external fun dispatchWorkgroupsIndirect(indirectBuffer: Buffer, indirectOffset: Long, ): Unit

    @FastNative
    @JvmName("end")
    public external fun end(): Unit

    @FastNative
    @JvmName("insertDebugMarker")
    public external fun insertDebugMarker(markerLabel: String, ): Unit

    @FastNative
    @JvmName("popDebugGroup")
    public external fun popDebugGroup(): Unit

    @FastNative
    @JvmName("pushDebugGroup")
    public external fun pushDebugGroup(groupLabel: String, ): Unit

    @FastNative
    @JvmName("setBindGroup")
    public external fun setBindGroup(groupIndex: Int, group: BindGroup? = null, dynamicOffsets: IntArray = intArrayOf(), ): Unit

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    @FastNative
    @JvmName("setPipeline")
    public external fun setPipeline(pipeline: ComputePipeline, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is ComputePassEncoder && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
