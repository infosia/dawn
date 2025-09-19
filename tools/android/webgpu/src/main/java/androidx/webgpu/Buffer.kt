package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Buffer(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("destroy")
    public external fun destroy(): Unit

    /**
    * Returns a const pointer to beginning of the mapped range. It must not be
    * written; writing to this range causes undefined behavior. See @ref
    * MappedRangeBehavior for error conditions and guarantees. This function is safe
    * to call inside spontaneous callbacks (see @ref CallbackReentrancy).  In Wasm, if
    * `memcpy`ing from this range, prefer using @ref wgpuBufferReadMappedRange instead
    * for better performance.
    * @param offset Byte offset relative to the beginning of the buffer.
    * @param size Byte size of the range to get. If this is @ref WGPU_WHOLE_MAP_SIZE, it defaults to
    * `buffer.size - offset`. The returned pointer is valid for exactly this many bytes.
    */
    @FastNative
    @JvmName("getConstMappedRange")
    public external fun getConstMappedRange(offset: Long = 0, size: Long = Constants.WHOLE_MAP_SIZE, ): ByteBuffer


    /**
    * Returns a mutable pointer to beginning of the mapped range. See @ref
    * MappedRangeBehavior for error conditions and guarantees. This function is safe
    * to call inside spontaneous callbacks (see @ref CallbackReentrancy).  In Wasm, if
    * `memcpy`ing into this range, prefer using @ref wgpuBufferWriteMappedRange
    * instead for better performance.
    * @param offset Byte offset relative to the beginning of the buffer.
    * @param size Byte size of the range to get. If this is @ref WGPU_WHOLE_MAP_SIZE, it defaults to
    * `buffer.size - offset`. The returned pointer is valid for exactly this many bytes.
    */
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

    /**
    * @param offset Byte offset relative to beginning of the buffer.
    * @param size Byte size of the region to map. If this is @ref WGPU_WHOLE_MAP_SIZE, it defaults to
    * `buffer.size - offset`.
    */
    @FastNative
    @JvmName("mapAsync")
    public external fun mapAsync(mode: MapMode, offset: Long, size: Long, callbackInfo: BufferMapCallbackInfo, ): Future

    /**
    * Copies a range of data from the buffer mapping into the provided destination
    * pointer. See @ref MappedRangeBehavior for error conditions and guarantees. This
    * function is safe to call inside spontaneous callbacks (see @ref
    * CallbackReentrancy).  In Wasm, this is more efficient than copying from a mapped
    * range into a `malloc`'d range.
    * @param offset Byte offset relative to the beginning of the buffer.
    * @param data Destination, to read buffer data into.
    * @return @ref WGPUStatus_Error if the copy did not occur.
    */
    @FastNative
    @JvmName("readMappedRange")
    public external fun readMappedRange(offset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Status

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    @FastNative
    @JvmName("unmap")
    public external fun unmap(): Unit

    /**
    * Copies a range of data from the provided source pointer into the buffer mapping.
    * See @ref MappedRangeBehavior for error conditions and guarantees. This function
    * is safe to call inside spontaneous callbacks (see @ref CallbackReentrancy).  In
    * Wasm, this is more efficient than copying from a `malloc`'d range into a mapped
    * range.
    * @param offset Byte offset relative to the beginning of the buffer.
    * @param data Source, to write buffer data from.
    * @return @ref WGPUStatus_Error if the copy did not occur.
    */
    @FastNative
    @JvmName("writeMappedRange")
    public external fun writeMappedRange(offset: Long, data: java.nio.ByteBuffer = java.nio.ByteBuffer.allocateDirect(0), ): Status

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Buffer && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
