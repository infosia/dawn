package androidx.webgpu

@JvmInline
public value class PredefinedColorSpace(public val value: Int) {
    public companion object {
        public val SRGB: PredefinedColorSpace = PredefinedColorSpace(0x00000001)
        public val DisplayP3: PredefinedColorSpace = PredefinedColorSpace(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "SRGB",
            0x00000002 to "DisplayP3",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
