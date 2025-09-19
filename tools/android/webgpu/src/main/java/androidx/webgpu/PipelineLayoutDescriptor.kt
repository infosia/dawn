package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class PipelineLayoutDescriptor(
    public var label: String? = null,
    public var bindGroupLayouts: Array<BindGroupLayout> = arrayOf(),
    public var immediateSize: Int = 0,
)
