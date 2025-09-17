package androidx.webgpu

public class RequestAdapterCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: RequestAdapterCallback,
)
