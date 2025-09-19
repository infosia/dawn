package androidx.webgpu


public fun interface UncapturedErrorCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onUncapturedError")
    public fun onUncapturedError(device: Device, type: ErrorType, message: String, );
}


internal class UncapturedErrorCallbackRunnable(
private val callback: UncapturedErrorCallback,
private val device: Device,
private val type: ErrorType,
private val message: String
) : Runnable {
    override fun run() {
        callback.onUncapturedError(device, type, message)
    }
}
