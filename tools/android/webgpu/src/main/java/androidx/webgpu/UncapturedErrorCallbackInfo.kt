package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class UncapturedErrorCallbackInfo(
    public var executor: Executor? = ForkJoinPool.commonPool(),
    public var callback: UncapturedErrorCallback? = null,
)
