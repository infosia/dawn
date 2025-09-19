package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Struct holding a future to wait on, and a `completed` boolean flag.
 */
public class FutureWaitInfo(
    /**
     * The future to wait on.
     */
    public var future: Future,
    /**
     * Whether or not the future completed.
     */
    public var completed: Boolean = false,
)
