package androidx.webgpu


/**
 * @param status @see QueueWorkDoneStatus.
 */
public fun interface QueueWorkDoneCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onQueueWorkDone")
    public fun onQueueWorkDone(status: QueueWorkDoneStatus, message: String, );
}


internal class QueueWorkDoneCallbackRunnable(
private val callback: QueueWorkDoneCallback,
private val status: QueueWorkDoneStatus,
private val message: String
) : Runnable {
    override fun run() {
        callback.onQueueWorkDone(status, message)
    }
}
