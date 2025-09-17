package androidx.webgpu

@JvmInline
public value class CallbackMode(public val value: Int) {
    public companion object {
        public val WaitAnyOnly: CallbackMode = CallbackMode(0x00000001)
        public val AllowProcessEvents: CallbackMode = CallbackMode(0x00000002)
        public val AllowSpontaneous: CallbackMode = CallbackMode(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "WaitAnyOnly",
            0x00000002 to "AllowProcessEvents",
            0x00000003 to "AllowSpontaneous",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
