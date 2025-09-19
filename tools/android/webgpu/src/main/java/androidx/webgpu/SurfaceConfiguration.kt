package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Options to @ref wgpuSurfaceConfigure for defining how a @ref WGPUSurface will be
 * rendered to and presented to the user. See @ref Surface-Configuration for more
 * details.
 */
public class SurfaceConfiguration(
    /**
     * The @ref WGPUDevice to use to render to surface's textures.
     */
    public var device: Device,
    /**
     * The @ref WGPUTextureFormat of the surface's textures.
     */
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    /**
     * The @ref WGPUTextureUsage of the surface's textures.
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
