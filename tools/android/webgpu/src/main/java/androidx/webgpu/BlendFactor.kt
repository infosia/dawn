package androidx.webgpu

@JvmInline
public value class BlendFactor(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: BlendFactor = BlendFactor(0x00000000)
        public val Zero: BlendFactor = BlendFactor(0x00000001)
        public val One: BlendFactor = BlendFactor(0x00000002)
        public val Src: BlendFactor = BlendFactor(0x00000003)
        public val OneMinusSrc: BlendFactor = BlendFactor(0x00000004)
        public val SrcAlpha: BlendFactor = BlendFactor(0x00000005)
        public val OneMinusSrcAlpha: BlendFactor = BlendFactor(0x00000006)
        public val Dst: BlendFactor = BlendFactor(0x00000007)
        public val OneMinusDst: BlendFactor = BlendFactor(0x00000008)
        public val DstAlpha: BlendFactor = BlendFactor(0x00000009)
        public val OneMinusDstAlpha: BlendFactor = BlendFactor(0x0000000a)
        public val SrcAlphaSaturated: BlendFactor = BlendFactor(0x0000000b)
        public val Constant: BlendFactor = BlendFactor(0x0000000c)
        public val OneMinusConstant: BlendFactor = BlendFactor(0x0000000d)
        public val Src1: BlendFactor = BlendFactor(0x0000000e)
        public val OneMinusSrc1: BlendFactor = BlendFactor(0x0000000f)
        public val Src1Alpha: BlendFactor = BlendFactor(0x00000010)
        public val OneMinusSrc1Alpha: BlendFactor = BlendFactor(0x00000011)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Zero",
            0x00000002 to "One",
            0x00000003 to "Src",
            0x00000004 to "OneMinusSrc",
            0x00000005 to "SrcAlpha",
            0x00000006 to "OneMinusSrcAlpha",
            0x00000007 to "Dst",
            0x00000008 to "OneMinusDst",
            0x00000009 to "DstAlpha",
            0x0000000a to "OneMinusDstAlpha",
            0x0000000b to "SrcAlphaSaturated",
            0x0000000c to "Constant",
            0x0000000d to "OneMinusConstant",
            0x0000000e to "Src1",
            0x0000000f to "OneMinusSrc1",
            0x00000010 to "Src1Alpha",
            0x00000011 to "OneMinusSrc1Alpha",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
