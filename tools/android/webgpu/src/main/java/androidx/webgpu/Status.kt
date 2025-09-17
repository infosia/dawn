package androidx.webgpu

@JvmInline
public value class Status(public val value: Int) {
    public companion object {
        public val Success: Status = Status(0x00000001)
        public val Error: Status = Status(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
