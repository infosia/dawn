package androidx.webgpu

@JvmInline
public value class CompositeAlphaMode(public val value: Int) {
    public companion object {
        public val Auto: CompositeAlphaMode = CompositeAlphaMode(0x00000000)
        public val Opaque: CompositeAlphaMode = CompositeAlphaMode(0x00000001)
        public val Premultiplied: CompositeAlphaMode = CompositeAlphaMode(0x00000002)
        public val Unpremultiplied: CompositeAlphaMode = CompositeAlphaMode(0x00000003)
        public val Inherit: CompositeAlphaMode = CompositeAlphaMode(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Auto",
            0x00000001 to "Opaque",
            0x00000002 to "Premultiplied",
            0x00000003 to "Unpremultiplied",
            0x00000004 to "Inherit",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
