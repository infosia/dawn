package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class ConstantEntry(
    public var key: String,
    public var value: Double,
)
