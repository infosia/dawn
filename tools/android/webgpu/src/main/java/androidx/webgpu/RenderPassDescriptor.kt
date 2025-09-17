package androidx.webgpu

public class RenderPassDescriptor(
    public var label: String? = null,
    public var colorAttachments: Array<RenderPassColorAttachment> = arrayOf(),
    public var depthStencilAttachment: RenderPassDepthStencilAttachment? = null,
    public var occlusionQuerySet: QuerySet? = null,
    public var timestampWrites: PassTimestampWrites? = null,
    public var renderPassMaxDrawCount: RenderPassMaxDrawCount? = null,
)
