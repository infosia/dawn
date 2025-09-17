package androidx.webgpu

@JvmInline
public value class VertexFormat(public val value: Int) {
    public companion object {
        public val Uint8: VertexFormat = VertexFormat(0x00000001)
        public val Uint8x2: VertexFormat = VertexFormat(0x00000002)
        public val Uint8x4: VertexFormat = VertexFormat(0x00000003)
        public val Sint8: VertexFormat = VertexFormat(0x00000004)
        public val Sint8x2: VertexFormat = VertexFormat(0x00000005)
        public val Sint8x4: VertexFormat = VertexFormat(0x00000006)
        public val Unorm8: VertexFormat = VertexFormat(0x00000007)
        public val Unorm8x2: VertexFormat = VertexFormat(0x00000008)
        public val Unorm8x4: VertexFormat = VertexFormat(0x00000009)
        public val Snorm8: VertexFormat = VertexFormat(0x0000000a)
        public val Snorm8x2: VertexFormat = VertexFormat(0x0000000b)
        public val Snorm8x4: VertexFormat = VertexFormat(0x0000000c)
        public val Uint16: VertexFormat = VertexFormat(0x0000000d)
        public val Uint16x2: VertexFormat = VertexFormat(0x0000000e)
        public val Uint16x4: VertexFormat = VertexFormat(0x0000000f)
        public val Sint16: VertexFormat = VertexFormat(0x00000010)
        public val Sint16x2: VertexFormat = VertexFormat(0x00000011)
        public val Sint16x4: VertexFormat = VertexFormat(0x00000012)
        public val Unorm16: VertexFormat = VertexFormat(0x00000013)
        public val Unorm16x2: VertexFormat = VertexFormat(0x00000014)
        public val Unorm16x4: VertexFormat = VertexFormat(0x00000015)
        public val Snorm16: VertexFormat = VertexFormat(0x00000016)
        public val Snorm16x2: VertexFormat = VertexFormat(0x00000017)
        public val Snorm16x4: VertexFormat = VertexFormat(0x00000018)
        public val Float16: VertexFormat = VertexFormat(0x00000019)
        public val Float16x2: VertexFormat = VertexFormat(0x0000001a)
        public val Float16x4: VertexFormat = VertexFormat(0x0000001b)
        public val Float32: VertexFormat = VertexFormat(0x0000001c)
        public val Float32x2: VertexFormat = VertexFormat(0x0000001d)
        public val Float32x3: VertexFormat = VertexFormat(0x0000001e)
        public val Float32x4: VertexFormat = VertexFormat(0x0000001f)
        public val Uint32: VertexFormat = VertexFormat(0x00000020)
        public val Uint32x2: VertexFormat = VertexFormat(0x00000021)
        public val Uint32x3: VertexFormat = VertexFormat(0x00000022)
        public val Uint32x4: VertexFormat = VertexFormat(0x00000023)
        public val Sint32: VertexFormat = VertexFormat(0x00000024)
        public val Sint32x2: VertexFormat = VertexFormat(0x00000025)
        public val Sint32x3: VertexFormat = VertexFormat(0x00000026)
        public val Sint32x4: VertexFormat = VertexFormat(0x00000027)
        public val Unorm10_10_10_2: VertexFormat = VertexFormat(0x00000028)
        public val Unorm8x4BGRA: VertexFormat = VertexFormat(0x00000029)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Uint8",
            0x00000002 to "Uint8x2",
            0x00000003 to "Uint8x4",
            0x00000004 to "Sint8",
            0x00000005 to "Sint8x2",
            0x00000006 to "Sint8x4",
            0x00000007 to "Unorm8",
            0x00000008 to "Unorm8x2",
            0x00000009 to "Unorm8x4",
            0x0000000a to "Snorm8",
            0x0000000b to "Snorm8x2",
            0x0000000c to "Snorm8x4",
            0x0000000d to "Uint16",
            0x0000000e to "Uint16x2",
            0x0000000f to "Uint16x4",
            0x00000010 to "Sint16",
            0x00000011 to "Sint16x2",
            0x00000012 to "Sint16x4",
            0x00000013 to "Unorm16",
            0x00000014 to "Unorm16x2",
            0x00000015 to "Unorm16x4",
            0x00000016 to "Snorm16",
            0x00000017 to "Snorm16x2",
            0x00000018 to "Snorm16x4",
            0x00000019 to "Float16",
            0x0000001a to "Float16x2",
            0x0000001b to "Float16x4",
            0x0000001c to "Float32",
            0x0000001d to "Float32x2",
            0x0000001e to "Float32x3",
            0x0000001f to "Float32x4",
            0x00000020 to "Uint32",
            0x00000021 to "Uint32x2",
            0x00000022 to "Uint32x3",
            0x00000023 to "Uint32x4",
            0x00000024 to "Sint32",
            0x00000025 to "Sint32x2",
            0x00000026 to "Sint32x3",
            0x00000027 to "Sint32x4",
            0x00000028 to "Unorm10_10_10_2",
            0x00000029 to "Unorm8x4BGRA",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
