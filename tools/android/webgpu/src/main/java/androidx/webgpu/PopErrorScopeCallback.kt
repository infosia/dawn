package androidx.webgpu


/**
* @param statusSee @ref WGPUPopErrorScopeStatus.
* @param typeThe type of the error caught by the scope, or @ref WGPUErrorType_NoError if there was
* none. If the `status` is not @ref WGPUPopErrorScopeStatus_Success, this is @ref
* WGPUErrorType_NoError.
* @param messageIf the `status` is not @ref WGPUPopErrorScopeStatus_Success **or** the `type` is not @ref
* WGPUErrorType_NoError, this is a non-empty @ref LocalizableHumanReadableMessageString;
* otherwise, this is an empty string.
*/
public fun interface PopErrorScopeCallback {
    @Suppress("INAPPLICABLE_JVM_NAME")
    @JvmName("onPopErrorScope")
    public fun onPopErrorScope(status: PopErrorScopeStatus, type: ErrorType, message: String, );
}
