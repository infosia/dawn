package androidx.webgpu

@JvmInline
public value class PrimitiveTopology(public val value: Int) {
    public companion object {
        public val Undefined: PrimitiveTopology = PrimitiveTopology(0x00000000)
        public val PointList: PrimitiveTopology = PrimitiveTopology(0x00000001)
        public val LineList: PrimitiveTopology = PrimitiveTopology(0x00000002)
        public val LineStrip: PrimitiveTopology = PrimitiveTopology(0x00000003)
        public val TriangleList: PrimitiveTopology = PrimitiveTopology(0x00000004)
        public val TriangleStrip: PrimitiveTopology = PrimitiveTopology(0x00000005)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "PointList",
            0x00000002 to "LineList",
            0x00000003 to "LineStrip",
            0x00000004 to "TriangleList",
            0x00000005 to "TriangleStrip",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
