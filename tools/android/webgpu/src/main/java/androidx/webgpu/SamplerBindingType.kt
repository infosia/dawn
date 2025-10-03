package androidx.webgpu

@JvmInline
public value class SamplerBindingType(public val value: Int) {
    public companion object {
        /**
         * Indicates that this @see SamplerBindingLayout member of its parent @see
         * BindGroupLayoutEntry is not used.
         */
        public val BindingNotUsed: SamplerBindingType = SamplerBindingType(0x00000000)
        /**
         * `1`. Indicates no value is passed for this argument.
         */
        public val Undefined: SamplerBindingType = SamplerBindingType(0x00000001)
        public val Filtering: SamplerBindingType = SamplerBindingType(0x00000002)
        public val NonFiltering: SamplerBindingType = SamplerBindingType(0x00000003)
        public val Comparison: SamplerBindingType = SamplerBindingType(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "BindingNotUsed",
            0x00000001 to "Undefined",
            0x00000002 to "Filtering",
            0x00000003 to "NonFiltering",
            0x00000004 to "Comparison",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
