package androidx.webgpu

@JvmInline
public value class TextureSampleType(public val value: Int) {
    public companion object {
        /**
         * Indicates that this @ref WGPUTextureBindingLayout member of its parent @ref
         * WGPUBindGroupLayoutEntry is not used. (See also @ref SentinelValues.)
         */
        public val BindingNotUsed: TextureSampleType = TextureSampleType(0x00000000)
        /**
         * `1`. Indicates no value is passed for this argument. See @ref SentinelValues.
         */
        public val Undefined: TextureSampleType = TextureSampleType(0x00000001)
        public val Float: TextureSampleType = TextureSampleType(0x00000002)
        public val UnfilterableFloat: TextureSampleType = TextureSampleType(0x00000003)
        public val Depth: TextureSampleType = TextureSampleType(0x00000004)
        public val Sint: TextureSampleType = TextureSampleType(0x00000005)
        public val Uint: TextureSampleType = TextureSampleType(0x00000006)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "BindingNotUsed",
            0x00000001 to "Undefined",
            0x00000002 to "Float",
            0x00000003 to "UnfilterableFloat",
            0x00000004 to "Depth",
            0x00000005 to "Sint",
            0x00000006 to "Uint",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
