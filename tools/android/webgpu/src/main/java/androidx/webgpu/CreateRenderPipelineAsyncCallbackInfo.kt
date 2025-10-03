package androidx.webgpu

public class CreateRenderPipelineAsyncCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: CreateRenderPipelineAsyncCallback,
)
