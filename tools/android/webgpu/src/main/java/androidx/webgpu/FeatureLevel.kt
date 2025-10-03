package androidx.webgpu

@JvmInline
public value class FeatureLevel(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: FeatureLevel = FeatureLevel(0x00000000)
        /**
         * "Compatibility" profile which can be supported on OpenGL ES 3.1 and D3D11.
         */
        public val Compatibility: FeatureLevel = FeatureLevel(0x00000001)
        /**
         * "Core" profile which can be supported on Vulkan/Metal/D3D12 (at least).
         */
        public val Core: FeatureLevel = FeatureLevel(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Compatibility",
            0x00000002 to "Core",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
