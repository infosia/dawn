package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class ComputePassDescriptor(
    public var label: String? = null,
    public var timestampWrites: PassTimestampWrites? = null,
)
