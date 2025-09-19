package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class TexelCopyBufferInfo(
    public var layout: TexelCopyBufferLayout,
    public var buffer: Buffer,
)
