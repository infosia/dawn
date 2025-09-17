package androidx.webgpu

@JvmInline
public value class AddressMode(public val value: Int) {
    public companion object {
        public val Undefined: AddressMode = AddressMode(0x00000000)
        public val ClampToEdge: AddressMode = AddressMode(0x00000001)
        public val Repeat: AddressMode = AddressMode(0x00000002)
        public val MirrorRepeat: AddressMode = AddressMode(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "ClampToEdge",
            0x00000002 to "Repeat",
            0x00000003 to "MirrorRepeat",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
