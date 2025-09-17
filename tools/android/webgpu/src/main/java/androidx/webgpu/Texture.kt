package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Texture(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("createView")
    public external fun createView(descriptor: TextureViewDescriptor? = null, ): TextureView

    @FastNative
    @JvmName("destroy")
    public external fun destroy(): Unit

    @FastNative
    @JvmName("getDepthOrArrayLayers")
    public external fun getDepthOrArrayLayers(): Int

    @get:JvmName("depthOrArrayLayers")
    public val depthOrArrayLayers: Int get() = getDepthOrArrayLayers()

    @FastNative
    @JvmName("getDimension")
    public external fun getDimension(): TextureDimension

    @get:JvmName("dimension")
    public val dimension: TextureDimension get() = getDimension()

    @FastNative
    @JvmName("getFormat")
    public external fun getFormat(): TextureFormat

    @get:JvmName("format")
    public val format: TextureFormat get() = getFormat()

    @FastNative
    @JvmName("getHeight")
    public external fun getHeight(): Int

    @get:JvmName("height")
    public val height: Int get() = getHeight()

    @FastNative
    @JvmName("getMipLevelCount")
    public external fun getMipLevelCount(): Int

    @get:JvmName("mipLevelCount")
    public val mipLevelCount: Int get() = getMipLevelCount()

    @FastNative
    @JvmName("getSampleCount")
    public external fun getSampleCount(): Int

    @get:JvmName("sampleCount")
    public val sampleCount: Int get() = getSampleCount()

    @FastNative
    @JvmName("getUsage")
    public external fun getUsage(): TextureUsage

    @get:JvmName("usage")
    public val usage: TextureUsage get() = getUsage()

    @FastNative
    @JvmName("getWidth")
    public external fun getWidth(): Int

    @get:JvmName("width")
    public val width: Int get() = getWidth()

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Texture && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
