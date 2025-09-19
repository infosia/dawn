package androidx.webgpu


public fun interface RequestAdapterCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestAdapter")
    public fun onRequestAdapter(status: RequestAdapterStatus, adapter: Adapter?, message: String, );
}


internal class RequestAdapterCallbackRunnable(
private val callback: RequestAdapterCallback,
private val status: RequestAdapterStatus,
private val adapter: Adapter?,
private val message: String
) : Runnable {
    override fun run() {
        callback.onRequestAdapter(status, adapter, message)
    }
}
