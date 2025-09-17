package androidx.webgpu

@JvmInline
public value class CompilationMessageType(public val value: Int) {
    public companion object {
        public val Error: CompilationMessageType = CompilationMessageType(0x00000001)
        public val Warning: CompilationMessageType = CompilationMessageType(0x00000002)
        public val Info: CompilationMessageType = CompilationMessageType(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Error",
            0x00000002 to "Warning",
            0x00000003 to "Info",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
