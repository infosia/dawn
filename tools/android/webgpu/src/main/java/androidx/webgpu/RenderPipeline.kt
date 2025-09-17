package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class RenderPipeline(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("getBindGroupLayout")
    public external fun getBindGroupLayout(groupIndex: Int, ): BindGroupLayout

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is RenderPipeline && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
