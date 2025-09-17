package androidx.webgpu

public class RequestDeviceCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: RequestDeviceCallback,
)
