package androidx.webgpu


/**
 * @param status See @ref WGPUQueueWorkDoneStatus.
 * @param message If the `status` is not @ref WGPUQueueWorkDoneStatus_Success, this is a non-empty @ref
 * LocalizableHumanReadableMessageString; otherwise, this is an empty string.
 */
public fun interface QueueWorkDoneCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onQueueWorkDone")
    public fun onQueueWorkDone(status: QueueWorkDoneStatus, message: String, );
}
