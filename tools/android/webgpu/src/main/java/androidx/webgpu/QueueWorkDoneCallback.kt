package androidx.webgpu


/**
 * @param status @see QueueWorkDoneStatus.
 */
public fun interface QueueWorkDoneCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onQueueWorkDone")
    public fun onQueueWorkDone(status: QueueWorkDoneStatus, message: String, );
}
