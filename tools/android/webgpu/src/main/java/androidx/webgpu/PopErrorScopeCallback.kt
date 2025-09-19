package androidx.webgpu


/**
 * @param status @see PopErrorScopeStatus.
 * @param type The type of the error caught by the scope, or @see ErrorType.NoError if there was none. If
 * the `status` is not @see PopErrorScopeStatus.Success, this is @see ErrorType.NoError.
 */
public fun interface PopErrorScopeCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onPopErrorScope")
    public fun onPopErrorScope(status: PopErrorScopeStatus, type: ErrorType, message: String, );
}


internal class PopErrorScopeCallbackRunnable(
private val callback: PopErrorScopeCallback,
private val status: PopErrorScopeStatus,
private val type: ErrorType,
private val message: String
) : Runnable {
    override fun run() {
        callback.onPopErrorScope(status, type, message)
    }
}
