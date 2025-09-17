package androidx.webgpu

@JvmInline
public value class MipmapFilterMode(public val value: Int) {
    public companion object {
        public val Undefined: MipmapFilterMode = MipmapFilterMode(0x00000000)
        public val Nearest: MipmapFilterMode = MipmapFilterMode(0x00000001)
        public val Linear: MipmapFilterMode = MipmapFilterMode(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Nearest",
            0x00000002 to "Linear",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
