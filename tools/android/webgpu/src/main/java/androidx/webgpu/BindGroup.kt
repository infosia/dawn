package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class BindGroup(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is BindGroup && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
