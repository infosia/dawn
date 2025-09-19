package androidx.webgpu


public fun interface BufferMapCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onBufferMap")
    public fun onBufferMap(status: MapAsyncStatus, message: String, );
}


internal class BufferMapCallbackRunnable(
private val callback: BufferMapCallback,
private val status: MapAsyncStatus,
private val message: String
) : Runnable {
    override fun run() {
        callback.onBufferMap(status, message)
    }
}
