package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class CommandBuffer(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is CommandBuffer && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
