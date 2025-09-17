package androidx.webgpu

@JvmInline
public value class TextureDimension(public val value: Int) {
    public companion object {
        public val Undefined: TextureDimension = TextureDimension(0x00000000)
        public val _1D: TextureDimension = TextureDimension(0x00000001)
        public val _2D: TextureDimension = TextureDimension(0x00000002)
        public val _3D: TextureDimension = TextureDimension(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "_1D",
            0x00000002 to "_2D",
            0x00000003 to "_3D",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
