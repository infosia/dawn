package androidx.webgpu

public class PipelineLayoutDescriptor(
    public var label: String? = null,
    public var bindGroupLayouts: Array<BindGroupLayout> = arrayOf(),
    public var immediateSize: Int = 0,
)
