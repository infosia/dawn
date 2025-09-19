package androidx.webgpu


/**
* @param status
* @param pipeline
* @param message
*/
public fun interface CreateComputePipelineAsyncCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCreateComputePipelineAsync")
    public fun onCreateComputePipelineAsync(status: CreatePipelineAsyncStatus, pipeline: ComputePipeline?, message: String, );
}
