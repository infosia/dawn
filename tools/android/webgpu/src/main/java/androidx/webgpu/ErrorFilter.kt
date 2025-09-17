package androidx.webgpu

@JvmInline
public value class ErrorFilter(public val value: Int) {
    public companion object {
        public val Validation: ErrorFilter = ErrorFilter(0x00000001)
        public val OutOfMemory: ErrorFilter = ErrorFilter(0x00000002)
        public val Internal: ErrorFilter = ErrorFilter(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Validation",
            0x00000002 to "OutOfMemory",
            0x00000003 to "Internal",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
