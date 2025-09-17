package androidx.webgpu

@JvmInline
public value class BufferMapState(public val value: Int) {
    public companion object {
        public val Unmapped: BufferMapState = BufferMapState(0x00000001)
        public val Pending: BufferMapState = BufferMapState(0x00000002)
        public val Mapped: BufferMapState = BufferMapState(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Unmapped",
            0x00000002 to "Pending",
            0x00000003 to "Mapped",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
