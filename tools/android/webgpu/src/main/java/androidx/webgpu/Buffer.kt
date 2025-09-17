package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Buffer(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("destroy")
    public external fun destroy(): Unit

    @FastNative
    @JvmName("getConstMappedRange")
    public external fun getConstMappedRange(offset: Long = 0, size: Long = Constants.WHOLE_MAP_SIZE, ): ByteBuffer


    @FastNative
    @JvmName("getMappedRange")
    public external fun getMappedRange(offset: Long = 0, size: Long = Constants.WHOLE_MAP_SIZE, ): ByteBuffer


    @FastNative
    @JvmName("getMapState")
    public external fun getMapState(): BufferMapState

    @get:JvmName("mapState")
    public val mapState: BufferMapState get() = getMapState()

    @FastNative
    @JvmName("getSize")
    public external fun getSize(): Long

    @get:JvmName("size")
    public val size: Long get() = getSize()

    @FastNative
    @JvmName("getUsage")
    public external fun getUsage(): BufferUsage

    @get:JvmName("usage")
    public val usage: BufferUsage get() = getUsage()

    @FastNative
    @JvmName("mapAsync")
    public external fun mapAsync(mode: MapMode, offset: Long, size: Long, callbackInfo: BufferMapCallbackInfo, ): Future

    @FastNative
    @JvmName("readMappedRange")
    public external fun readMappedRange(offset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Status

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    @FastNative
    @JvmName("unmap")
    public external fun unmap(): Unit

    @FastNative
    @JvmName("writeMappedRange")
    public external fun writeMappedRange(offset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Status

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Buffer && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
