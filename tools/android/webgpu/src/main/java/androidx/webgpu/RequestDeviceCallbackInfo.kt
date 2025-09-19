package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class RequestDeviceCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var executor: Executor? = ForkJoinPool.commonPool(),
    public var callback: RequestDeviceCallback,
)
