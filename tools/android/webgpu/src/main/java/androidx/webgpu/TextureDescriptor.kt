package androidx.webgpu

public class TextureDescriptor(
    public var label: String? = null,
    @get:JvmName("getUsage")
    public var usage: TextureUsage,
    @get:JvmName("getDimension")
    public var dimension: TextureDimension = TextureDimension._2D,
    public var size: Extent3D,
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    public var mipLevelCount: Int = 1,
    public var sampleCount: Int = 1,
    @get:JvmName("getViewFormats")
    public var viewFormats: Array<TextureFormat> = arrayOf(),
)
