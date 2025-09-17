package androidx.webgpu

@JvmInline
public value class AdapterType(public val value: Int) {
    public companion object {
        public val DiscreteGPU: AdapterType = AdapterType(0x00000001)
        public val IntegratedGPU: AdapterType = AdapterType(0x00000002)
        public val CPU: AdapterType = AdapterType(0x00000003)
        public val Unknown: AdapterType = AdapterType(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "DiscreteGPU",
            0x00000002 to "IntegratedGPU",
            0x00000003 to "CPU",
            0x00000004 to "Unknown",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
