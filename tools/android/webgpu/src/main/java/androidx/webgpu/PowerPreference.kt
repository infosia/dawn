package androidx.webgpu

@JvmInline
public value class PowerPreference(public val value: Int) {
    public companion object {
        public val Undefined: PowerPreference = PowerPreference(0x00000000)
        public val LowPower: PowerPreference = PowerPreference(0x00000001)
        public val HighPerformance: PowerPreference = PowerPreference(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "LowPower",
            0x00000002 to "HighPerformance",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
