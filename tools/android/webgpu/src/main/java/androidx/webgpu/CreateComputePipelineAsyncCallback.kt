package androidx.webgpu


public fun interface CreateComputePipelineAsyncCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCreateComputePipelineAsync")
    public fun onCreateComputePipelineAsync(status: CreatePipelineAsyncStatus, pipeline: ComputePipeline?, message: String, );
}


internal class CreateComputePipelineAsyncCallbackRunnable(
private val callback: CreateComputePipelineAsyncCallback,
private val status: CreatePipelineAsyncStatus,
private val pipeline: ComputePipeline?,
private val message: String
) : Runnable {
    override fun run() {
        callback.onCreateComputePipelineAsync(status, pipeline, message)
    }
}
