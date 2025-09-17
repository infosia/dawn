package androidx.webgpu

@JvmInline
public value class FrontFace(public val value: Int) {
    public companion object {
        public val Undefined: FrontFace = FrontFace(0x00000000)
        public val CCW: FrontFace = FrontFace(0x00000001)
        public val CW: FrontFace = FrontFace(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "CCW",
            0x00000002 to "CW",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
