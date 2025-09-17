package androidx.webgpu


public fun interface BufferMapCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onBufferMap")
    public fun onBufferMap(status: MapAsyncStatus, message: String, );
}
