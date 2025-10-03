package androidx.webgpu

@JvmInline
public value class IndexFormat(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: IndexFormat = IndexFormat(0x00000000)
        public val Uint16: IndexFormat = IndexFormat(0x00000001)
        public val Uint32: IndexFormat = IndexFormat(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Uint16",
            0x00000002 to "Uint32",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
