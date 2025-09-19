package androidx.webgpu

@JvmInline
public value class PopErrorScopeStatus(public val value: Int) {
    public companion object {
        /**
         * The error scope stack was successfully popped and a result was reported.
         */
        public val Success: PopErrorScopeStatus = PopErrorScopeStatus(0x00000001)
        public val CallbackCancelled: PopErrorScopeStatus = PopErrorScopeStatus(0x00000002)
        /**
         * The error scope stack could not be popped, because it was empty.
         */
        public val Error: PopErrorScopeStatus = PopErrorScopeStatus(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
