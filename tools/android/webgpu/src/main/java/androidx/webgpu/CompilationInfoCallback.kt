package androidx.webgpu


public fun interface CompilationInfoCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCompilationInfo")
    public fun onCompilationInfo(status: CompilationInfoRequestStatus, compilationInfo: CompilationInfo, );
}


internal class CompilationInfoCallbackRunnable(
private val callback: CompilationInfoCallback,
private val status: CompilationInfoRequestStatus,
private val compilationInfo: CompilationInfo
) : Runnable {
    override fun run() {
        callback.onCompilationInfo(status, compilationInfo)
    }
}
