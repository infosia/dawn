package androidx.webgpu


public fun interface CreateRenderPipelineAsyncCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCreateRenderPipelineAsync")
    public fun onCreateRenderPipelineAsync(status: CreatePipelineAsyncStatus, pipeline: RenderPipeline?, message: String, );
}


internal class CreateRenderPipelineAsyncCallbackRunnable(
private val callback: CreateRenderPipelineAsyncCallback,
private val status: CreatePipelineAsyncStatus,
private val pipeline: RenderPipeline?,
private val message: String
) : Runnable {
    override fun run() {
        callback.onCreateRenderPipelineAsync(status, pipeline, message)
    }
}
