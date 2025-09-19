package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class ComputePipelineDescriptor(
    public var label: String? = null,
    public var layout: PipelineLayout? = null,
    public var compute: ComputeState,
)
