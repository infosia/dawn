package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SurfaceCapabilities(
    /**
     * The bit set of supported @see TextureUsage bits. Guaranteed to contain @see
     * TextureUsage.RenderAttachment.
     */
    @get:JvmName("getUsages")
    public var usages: TextureUsage,
    /**
     * A list of supported @see TextureFormat values, in order of preference.
     */
    @get:JvmName("getFormats")
    public var formats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getPresentModes")
    public var presentModes: Array<PresentMode> = arrayOf(),
    @get:JvmName("getAlphaModes")
    public var alphaModes: Array<CompositeAlphaMode> = arrayOf(),
)
