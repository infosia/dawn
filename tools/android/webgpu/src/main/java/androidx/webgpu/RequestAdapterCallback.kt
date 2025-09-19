package androidx.webgpu


/**
* @param status
* @param adapter
* @param message
*/
public fun interface RequestAdapterCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestAdapter")
    public fun onRequestAdapter(status: RequestAdapterStatus, adapter: Adapter?, message: String, );
}
