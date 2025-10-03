package androidx.webgpu

public class VertexState(
    public var module: ShaderModule,
    public var entryPoint: String? = null,
    public var constants: Array<ConstantEntry> = arrayOf(),
    public var buffers: Array<VertexBufferLayout> = arrayOf(),
)
