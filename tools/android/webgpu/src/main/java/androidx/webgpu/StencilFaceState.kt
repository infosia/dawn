package androidx.webgpu

public class StencilFaceState(
    @get:JvmName("getCompare")
    public var compare: CompareFunction = CompareFunction.Always,
    @get:JvmName("getFailOp")
    public var failOp: StencilOperation = StencilOperation.Keep,
    @get:JvmName("getDepthFailOp")
    public var depthFailOp: StencilOperation = StencilOperation.Keep,
    @get:JvmName("getPassOp")
    public var passOp: StencilOperation = StencilOperation.Keep,
)
