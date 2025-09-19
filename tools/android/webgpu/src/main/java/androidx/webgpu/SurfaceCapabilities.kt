package androidx.webgpu

/**
 * Filled by @ref wgpuSurfaceGetCapabilities with what's supported for @ref
 * wgpuSurfaceConfigure for a pair of @ref WGPUSurface and @ref WGPUAdapter.
 */
public class SurfaceCapabilities(
    /**
     * The bit set of supported @ref WGPUTextureUsage bits. Guaranteed to contain @ref
     * WGPUTextureUsage_RenderAttachment.
     */
    @get:JvmName("getUsages")
    public var usages: TextureUsage,
    /**
     * A list of supported @ref WGPUTextureFormat values, in order of preference.
     */
    @get:JvmName("getFormats")
    public var formats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getPresentModes")
    public var presentModes: Array<PresentMode> = arrayOf(),
    @get:JvmName("getAlphaModes")
    public var alphaModes: Array<CompositeAlphaMode> = arrayOf(),
)
