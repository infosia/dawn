package androidx.webgpu

public class BlendComponent(
    /**
     * If set to @ref WGPUBlendOperation_Undefined, [defaults](@ref SentinelValues) to
     * @ref WGPUBlendOperation_Add.
     */
    @get:JvmName("getOperation")
    public var operation: BlendOperation = BlendOperation.Add,
    @get:JvmName("getSrcFactor")
    public var srcFactor: BlendFactor = BlendFactor.One,
    @get:JvmName("getDstFactor")
    public var dstFactor: BlendFactor = BlendFactor.Zero,
)
