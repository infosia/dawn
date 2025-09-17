package androidx.webgpu

public class SurfaceCapabilities(
    @get:JvmName("getUsages")
    public var usages: TextureUsage,
    @get:JvmName("getFormats")
    public var formats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getPresentModes")
    public var presentModes: Array<PresentMode> = arrayOf(),
    @get:JvmName("getAlphaModes")
    public var alphaModes: Array<CompositeAlphaMode> = arrayOf(),
)
