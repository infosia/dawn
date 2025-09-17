package androidx.webgpu


public fun interface RequestDeviceCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onRequestDevice")
    public fun onRequestDevice(status: RequestDeviceStatus, device: Device?, message: String, );
}
