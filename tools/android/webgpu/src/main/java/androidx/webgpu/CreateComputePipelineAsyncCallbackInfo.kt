package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class CreateComputePipelineAsyncCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var executor: Executor? = ForkJoinPool.commonPool(),
    public var callback: CreateComputePipelineAsyncCallback,
)
