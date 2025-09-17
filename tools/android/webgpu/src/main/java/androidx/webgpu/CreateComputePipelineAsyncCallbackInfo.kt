package androidx.webgpu

public class CreateComputePipelineAsyncCallbackInfo(
    @get:JvmName("getMode")
    public var mode: CallbackMode,
    public var callback: CreateComputePipelineAsyncCallback,
)
