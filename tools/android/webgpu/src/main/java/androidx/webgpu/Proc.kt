package androidx.webgpu


public fun interface Proc {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("on")
    public fun on();
}


internal class ProcRunnable(
private val callback: Proc,
) : Runnable {
    override fun run() {
        callback.on()
    }
}
