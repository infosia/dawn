package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BindGroupLayoutDescriptor(
    public var label: String? = null,
    public var entries: Array<BindGroupLayoutEntry> = arrayOf(),
)
