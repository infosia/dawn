package androidx.webgpu


/**
 * @param compilationInfo This argument contains multiple @ref ImplementationAllocatedStructChain roots. Arbitrary
 * chains must be handled gracefully by the application!
 */
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
