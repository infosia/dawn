package androidx.webgpu

@JvmInline
public value class TextureViewDimension(public val value: Int) {
    public companion object {
        public val Undefined: TextureViewDimension = TextureViewDimension(0x00000000)
        public val _1D: TextureViewDimension = TextureViewDimension(0x00000001)
        public val _2D: TextureViewDimension = TextureViewDimension(0x00000002)
        public val _2DArray: TextureViewDimension = TextureViewDimension(0x00000003)
        public val Cube: TextureViewDimension = TextureViewDimension(0x00000004)
        public val CubeArray: TextureViewDimension = TextureViewDimension(0x00000005)
        public val _3D: TextureViewDimension = TextureViewDimension(0x00000006)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "_1D",
            0x00000002 to "_2D",
            0x00000003 to "_2DArray",
            0x00000004 to "Cube",
            0x00000005 to "CubeArray",
            0x00000006 to "_3D",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
