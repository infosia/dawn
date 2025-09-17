package androidx.webgpu

public class BlendComponent(
    @get:JvmName("getOperation")
    public var operation: BlendOperation = BlendOperation.Add,
    @get:JvmName("getSrcFactor")
    public var srcFactor: BlendFactor = BlendFactor.One,
    @get:JvmName("getDstFactor")
    public var dstFactor: BlendFactor = BlendFactor.Zero,
)
