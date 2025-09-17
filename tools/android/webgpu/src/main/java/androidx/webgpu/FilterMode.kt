package androidx.webgpu

@JvmInline
public value class FilterMode(public val value: Int) {
    public companion object {
        public val Undefined: FilterMode = FilterMode(0x00000000)
        public val Nearest: FilterMode = FilterMode(0x00000001)
        public val Linear: FilterMode = FilterMode(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Nearest",
            0x00000002 to "Linear",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
