package androidx.webgpu

@JvmInline
public value class OptionalBool(public val value: Int) {
    public companion object {
        public val False: OptionalBool = OptionalBool(0x00000000)
        public val True: OptionalBool = OptionalBool(0x00000001)
        public val Undefined: OptionalBool = OptionalBool(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "False",
            0x00000001 to "True",
            0x00000002 to "Undefined",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
