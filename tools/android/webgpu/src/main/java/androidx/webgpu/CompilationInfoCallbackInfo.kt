package androidx.webgpu

public class CompilationInfoCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: CompilationInfoCallback,
)
