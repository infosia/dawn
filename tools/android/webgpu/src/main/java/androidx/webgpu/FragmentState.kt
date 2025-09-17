package androidx.webgpu

public class FragmentState(
    public var module: ShaderModule,
    public var entryPoint: String? = null,
    public var constants: Array<ConstantEntry> = arrayOf(),
    public var targets: Array<ColorTargetState> = arrayOf(),
)
