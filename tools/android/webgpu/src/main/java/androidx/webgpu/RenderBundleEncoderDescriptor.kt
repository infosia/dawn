package androidx.webgpu

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
