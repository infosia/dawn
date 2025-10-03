package androidx.webgpu

public class ComputeState(
    public var module: ShaderModule,
    public var entryPoint: String? = null,
    public var constants: Array<ConstantEntry> = arrayOf(),
)
