package androidx.webgpu

public class StencilFaceState(
    /**
     * If set to @see CompareFunction.Undefined, defaults to @see
     * CompareFunction.Always.
     */
    @get:JvmName("getCompare")
    public var compare: CompareFunction = CompareFunction.Always,
    @get:JvmName("getFailOp")
    public var failOp: StencilOperation = StencilOperation.Keep,
    @get:JvmName("getDepthFailOp")
    public var depthFailOp: StencilOperation = StencilOperation.Keep,
    @get:JvmName("getPassOp")
    public var passOp: StencilOperation = StencilOperation.Keep,
)
