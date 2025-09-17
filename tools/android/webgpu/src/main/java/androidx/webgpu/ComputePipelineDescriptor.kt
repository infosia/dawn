package androidx.webgpu

public class ComputePipelineDescriptor(
    public var label: String? = null,
    public var layout: PipelineLayout? = null,
    public var compute: ComputeState,
)
