package androidx.webgpu


/**
* @param status
* @param device
* @param message
*/
public fun interface RequestDeviceCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestDevice")
    public fun onRequestDevice(status: RequestDeviceStatus, device: Device?, message: String, );
}
