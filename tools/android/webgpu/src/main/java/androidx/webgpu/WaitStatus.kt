package androidx.webgpu

@JvmInline
public value class WaitStatus(public val value: Int) {
    public companion object {
        /**
         * At least one WGPUFuture completed successfully.
         */
        public val Success: WaitStatus = WaitStatus(0x00000001)
        /**
         * The wait operation succeeded, but no WGPUFutures completed within the timeout.
         */
        public val TimedOut: WaitStatus = WaitStatus(0x00000002)
        public val Error: WaitStatus = WaitStatus(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "TimedOut",
            0x00000003 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
