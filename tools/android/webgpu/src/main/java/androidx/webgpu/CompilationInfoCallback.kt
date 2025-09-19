package androidx.webgpu


/**
* @param status
* @param compilationInfoThis argument contains multiple @ref ImplementationAllocatedStructChain roots. Arbitrary
* chains must be handled gracefully by the application!
*/
public fun interface CompilationInfoCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCompilationInfo")
    public fun onCompilationInfo(status: CompilationInfoRequestStatus, compilationInfo: CompilationInfo, );
}
