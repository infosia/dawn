package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class RenderPassMaxDrawCount(
    public var maxDrawCount: Long = 50000000,
)
