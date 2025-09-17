package androidx.webgpu

public class RenderPassDepthStencilAttachment(
    public var view: TextureView,
    @get:JvmName("getDepthLoadOp")
    public var depthLoadOp: LoadOp = LoadOp.Undefined,
    @get:JvmName("getDepthStoreOp")
    public var depthStoreOp: StoreOp = StoreOp.Undefined,
    public var depthClearValue: Float = 0.0f,
    public var depthReadOnly: Boolean = false,
    @get:JvmName("getStencilLoadOp")
    public var stencilLoadOp: LoadOp = LoadOp.Undefined,
    @get:JvmName("getStencilStoreOp")
    public var stencilStoreOp: StoreOp = StoreOp.Undefined,
    public var stencilClearValue: Int = 0,
    public var stencilReadOnly: Boolean = false,
)
