package androidx.webgpu


public fun interface CreateRenderPipelineAsyncCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCreateRenderPipelineAsync")
    public fun onCreateRenderPipelineAsync(status: CreatePipelineAsyncStatus, pipeline: RenderPipeline?, message: String, );
}
