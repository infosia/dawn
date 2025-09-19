package androidx.webgpu

@JvmInline
public value class TextureAspect(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: TextureAspect = TextureAspect(0x00000000)
        public val All: TextureAspect = TextureAspect(0x00000001)
        public val StencilOnly: TextureAspect = TextureAspect(0x00000002)
        public val DepthOnly: TextureAspect = TextureAspect(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "All",
            0x00000002 to "StencilOnly",
            0x00000003 to "DepthOnly",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
