package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class TexelCopyBufferLayout(
    public var offset: Long = 0,
    public var bytesPerRow: Int = Constants.COPY_STRIDE_UNDEFINED,
    public var rowsPerImage: Int = Constants.COPY_STRIDE_UNDEFINED,
)
