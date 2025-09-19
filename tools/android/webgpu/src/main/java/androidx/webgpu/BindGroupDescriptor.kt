package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BindGroupDescriptor(
    public var label: String? = null,
    public var layout: BindGroupLayout,
    public var entries: Array<BindGroupEntry> = arrayOf(),
)
