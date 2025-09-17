package androidx.webgpu

public class PopErrorScopeCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: PopErrorScopeCallback,
)
