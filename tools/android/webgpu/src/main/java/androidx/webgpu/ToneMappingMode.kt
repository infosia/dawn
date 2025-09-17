package androidx.webgpu

@JvmInline
public value class ToneMappingMode(public val value: Int) {
    public companion object {
        public val Standard: ToneMappingMode = ToneMappingMode(0x00000001)
        public val Extended: ToneMappingMode = ToneMappingMode(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Standard",
            0x00000002 to "Extended",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
