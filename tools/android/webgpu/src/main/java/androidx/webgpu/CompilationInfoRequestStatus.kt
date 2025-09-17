package androidx.webgpu

@JvmInline
public value class CompilationInfoRequestStatus(public val value: Int) {
    public companion object {
        public val Success: CompilationInfoRequestStatus = CompilationInfoRequestStatus(0x00000001)
        public val CallbackCancelled: CompilationInfoRequestStatus = CompilationInfoRequestStatus(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
