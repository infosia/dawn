package androidx.webgpu


public fun interface QueueWorkDoneCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onQueueWorkDone")
    public fun onQueueWorkDone(status: QueueWorkDoneStatus, message: String, );
}
