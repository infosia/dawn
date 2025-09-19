package androidx.webgpu

@JvmInline
public value class CallbackMode(public val value: Int) {
    public companion object {
        public val WaitAnyOnly: CallbackMode = CallbackMode(0x00000001)
        public val AllowProcessEvents: CallbackMode = CallbackMode(0x00000002)
        /**
         * Callbacks created with `CallbackMode.AllowSpontaneous`: - fire for the same
         * reasons as callbacks created with `CallbackMode.AllowProcessEvents` - **may**
         * fire spontaneously on an arbitrary or application thread, when the WebGPU
         * implementations discovers that the asynchronous operation is complete.
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
