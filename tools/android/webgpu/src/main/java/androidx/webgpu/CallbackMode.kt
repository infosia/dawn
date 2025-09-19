package androidx.webgpu

@JvmInline
public value class CallbackMode(public val value: Int) {
    public companion object {
        /**
         * Callbacks created with `WGPUCallbackMode_WaitAnyOnly`: - fire when the
         * asynchronous operation's future is passed to a call to @ref wgpuInstanceWaitAny
         * AND the operation has already completed or it completes inside the call to @ref
         * wgpuInstanceWaitAny.
         */
        public val WaitAnyOnly: CallbackMode = CallbackMode(0x00000001)
        /**
         * Callbacks created with `WGPUCallbackMode_AllowProcessEvents`: - fire for the
         * same reasons as callbacks created with `WGPUCallbackMode_WaitAnyOnly` - fire
         * inside a call to @ref wgpuInstanceProcessEvents if the asynchronous operation is
         * complete.
         */
        public val AllowProcessEvents: CallbackMode = CallbackMode(0x00000002)
        /**
         * Callbacks created with `WGPUCallbackMode_AllowSpontaneous`: - fire for the same
         * reasons as callbacks created with `WGPUCallbackMode_AllowProcessEvents` -
         * **may** fire spontaneously on an arbitrary or application thread, when the
         * WebGPU implementations discovers that the asynchronous operation is complete.
         * Implementations _should_ fire spontaneous callbacks as soon as possible.  @note
         * Because spontaneous callbacks may fire at an arbitrary time on an arbitrary
         * thread, applications should take extra care when acquiring locks or mutating
         * state inside the callback. It undefined behavior to re-entrantly call into the
         * webgpu.h API if the callback fires while inside the callstack of another
         * webgpu.h function that is not `wgpuInstanceWaitAny` or
         * `wgpuInstanceProcessEvents`.
         */
        public val AllowSpontaneous: CallbackMode = CallbackMode(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "WaitAnyOnly",
            0x00000002 to "AllowProcessEvents",
            0x00000003 to "AllowSpontaneous",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
