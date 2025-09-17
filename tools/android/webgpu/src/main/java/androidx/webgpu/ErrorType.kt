package androidx.webgpu

@JvmInline
public value class ErrorType(public val value: Int) {
    public companion object {
        public val NoError: ErrorType = ErrorType(0x00000001)
        public val Validation: ErrorType = ErrorType(0x00000002)
        public val OutOfMemory: ErrorType = ErrorType(0x00000003)
        public val Internal: ErrorType = ErrorType(0x00000004)
        public val Unknown: ErrorType = ErrorType(0x00000005)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "NoError",
            0x00000002 to "Validation",
            0x00000003 to "OutOfMemory",
            0x00000004 to "Internal",
            0x00000005 to "Unknown",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
