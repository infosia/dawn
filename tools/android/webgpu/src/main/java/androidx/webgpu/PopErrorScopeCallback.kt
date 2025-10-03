package androidx.webgpu


public fun interface PopErrorScopeCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onPopErrorScope")
    public fun onPopErrorScope(status: PopErrorScopeStatus, type: ErrorType, message: String, );
}
