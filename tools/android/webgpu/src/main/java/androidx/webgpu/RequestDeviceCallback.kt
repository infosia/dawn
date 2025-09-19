package androidx.webgpu


public fun interface RequestDeviceCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestDevice")
    public fun onRequestDevice(status: RequestDeviceStatus, device: Device?, message: String, );
}


internal class RequestDeviceCallbackRunnable(
private val callback: RequestDeviceCallback,
private val status: RequestDeviceStatus,
private val device: Device?,
private val message: String
) : Runnable {
    override fun run() {
        callback.onRequestDevice(status, device, message)
    }
}
