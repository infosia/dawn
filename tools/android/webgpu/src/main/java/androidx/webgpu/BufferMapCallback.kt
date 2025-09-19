package androidx.webgpu


/**
* @param status
* @param message
*/
public fun interface BufferMapCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onBufferMap")
    public fun onBufferMap(status: MapAsyncStatus, message: String, );
}
