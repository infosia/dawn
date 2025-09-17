package androidx.webgpu

@JvmInline
public value class PopErrorScopeStatus(public val value: Int) {
    public companion object {
        public val Success: PopErrorScopeStatus = PopErrorScopeStatus(0x00000001)
        public val CallbackCancelled: PopErrorScopeStatus = PopErrorScopeStatus(0x00000002)
        public val Error: PopErrorScopeStatus = PopErrorScopeStatus(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
