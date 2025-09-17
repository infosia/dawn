package androidx.webgpu

public class SurfaceConfiguration(
    public var device: Device,
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    @get:JvmName("getUsage")
    public var usage: TextureUsage = TextureUsage.RenderAttachment,
    public var width: Int,
    public var height: Int,
    @get:JvmName("getViewFormats")
    public var viewFormats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getAlphaMode")
    public var alphaMode: CompositeAlphaMode = CompositeAlphaMode.Auto,
    @get:JvmName("getPresentMode")
    public var presentMode: PresentMode = PresentMode.Fifo,
)
