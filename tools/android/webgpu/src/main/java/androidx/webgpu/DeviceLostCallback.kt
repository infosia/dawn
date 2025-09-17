package androidx.webgpu


public fun interface DeviceLostCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onDeviceLost")
    public fun onDeviceLost(device: Device, reason: DeviceLostReason, message: String, );
}
