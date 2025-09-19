package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class InstanceLimits(
    public var timedWaitAnyMaxCount: Long = 0,
)
