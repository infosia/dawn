package androidx.webgpu

public class QueueWorkDoneCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: QueueWorkDoneCallback,
)
