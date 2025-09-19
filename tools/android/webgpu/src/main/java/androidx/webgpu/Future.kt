package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Opaque handle to an asynchronous operation. See @ref Asynchronous-Operations for
 * more information.
 */
public class Future(
    /**
     * Opaque id of the @ref WGPUFuture
     */
    public var id: Long,
)
