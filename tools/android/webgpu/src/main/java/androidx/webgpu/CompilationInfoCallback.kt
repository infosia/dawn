package androidx.webgpu


public fun interface CompilationInfoCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onCompilationInfo")
    public fun onCompilationInfo(status: CompilationInfoRequestStatus, compilationInfo: CompilationInfo, );
}
