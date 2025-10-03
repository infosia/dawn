package androidx.webgpu

@JvmInline
public value class RequestAdapterStatus(public val value: Int) {
    public companion object {
        public val Success: RequestAdapterStatus = RequestAdapterStatus(0x00000001)
        public val CallbackCancelled: RequestAdapterStatus = RequestAdapterStatus(0x00000002)
        public val Unavailable: RequestAdapterStatus = RequestAdapterStatus(0x00000003)
        public val Error: RequestAdapterStatus = RequestAdapterStatus(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "Unavailable",
            0x00000004 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
