package androidx.webgpu

@JvmInline
public value class FeatureLevel(public val value: Int) {
    public companion object {
        public val Undefined: FeatureLevel = FeatureLevel(0x00000000)
        public val Compatibility: FeatureLevel = FeatureLevel(0x00000001)
        public val Core: FeatureLevel = FeatureLevel(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Compatibility",
            0x00000002 to "Core",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
