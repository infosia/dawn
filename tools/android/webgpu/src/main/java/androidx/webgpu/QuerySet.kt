package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class QuerySet(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("destroy")
    public external fun destroy(): Unit

    @FastNative
    @JvmName("getCount")
    public external fun getCount(): Int

    @get:JvmName("count")
    public val count: Int get() = getCount()

    @FastNative
    @JvmName("getType")
    public external fun getType(): QueryType

    @get:JvmName("type")
    public val type: QueryType get() = getType()

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is QuerySet && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
