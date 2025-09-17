package androidx.webgpu

public class BufferMapCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: BufferMapCallback,
)
