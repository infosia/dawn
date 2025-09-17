package androidx.webgpu


public fun interface RequestAdapterCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestAdapter")
    public fun onRequestAdapter(status: RequestAdapterStatus, adapter: Adapter?, message: String, );
}
