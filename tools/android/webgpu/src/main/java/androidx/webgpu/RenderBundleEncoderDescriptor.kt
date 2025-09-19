package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class RenderBundleEncoderDescriptor(
    public var label: String? = null,
    @get:JvmName("getColorFormats")
    public var colorFormats: Array<TextureFormat> = arrayOf(),
    @get:JvmName("getDepthStencilFormat")
    public var depthStencilFormat: TextureFormat = TextureFormat.Undefined,
    public var sampleCount: Int = 1,
    public var depthReadOnly: Boolean = false,
    public var stencilReadOnly: Boolean = false,
)
