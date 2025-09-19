package androidx.webgpu


/**
 * @param status @see PopErrorScopeStatus.
 * @param type The type of the error caught by the scope, or @see ErrorType.NoError if there was none. If
 * the `status` is not @see PopErrorScopeStatus.Success, this is @see ErrorType.NoError .
 */
public fun interface PopErrorScopeCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onPopErrorScope")
    public fun onPopErrorScope(status: PopErrorScopeStatus, type: ErrorType, message: String, );
}
