package androidx.webgpu

public class SurfaceConfiguration(
    /**
     * The @see Device to use to render to surface's textures.
     */
    public var device: Device,
    /**
     * The @see TextureFormat of the surface's textures.
     */
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    /**
     * The @see TextureUsage of the surface's textures.
     */
    @get:JvmName("getUsage")
    public var usage: TextureUsage = TextureUsage.RenderAttachment,
    /**
     * The width of the surface's textures.
     */
    public var width: Int,
    /**
     * The height of the surface's textures.
     */
    public var height: Int,
    @get:JvmName("getViewFormats")
    public var viewFormats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getAlphaMode")
    public var alphaMode: CompositeAlphaMode = CompositeAlphaMode.Auto,
    @get:JvmName("getPresentMode")
    public var presentMode: PresentMode = PresentMode.Fifo,
)
