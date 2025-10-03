package androidx.webgpu

@JvmInline
public value class CullMode(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: CullMode = CullMode(0x00000000)
        public val None: CullMode = CullMode(0x00000001)
        public val Front: CullMode = CullMode(0x00000002)
        public val Back: CullMode = CullMode(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "None",
            0x00000002 to "Front",
            0x00000003 to "Back",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
