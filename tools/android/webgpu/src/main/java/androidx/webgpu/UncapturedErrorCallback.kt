package androidx.webgpu


/**
* @param device
* @param type
* @param message
*/
public fun interface UncapturedErrorCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onUncapturedError")
    public fun onUncapturedError(device: Device, type: ErrorType, message: String, );
}
