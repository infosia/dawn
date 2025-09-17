package androidx.webgpu

public class DeviceLostCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: DeviceLostCallback? = null,
)
