package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class Future(
    /**
     * Opaque id of the @see Future
     */
    public var id: Long,
)
