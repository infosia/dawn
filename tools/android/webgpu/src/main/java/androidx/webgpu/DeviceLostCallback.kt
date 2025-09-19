package androidx.webgpu


/**
* @param device Pointer to the device which was lost. This is always a non-null pointer. The pointed-to
* @ref WGPUDevice will be null if, and only if, either: (1) The `reason` is @ref
* WGPUDeviceLostReason_FailedCreation. (2) The last ref of the device has been (or is being)
* released: see @ref DeviceRelease.
*/
public fun interface DeviceLostCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onDeviceLost")
    public fun onDeviceLost(device: Device, reason: DeviceLostReason, message: String, );
}
