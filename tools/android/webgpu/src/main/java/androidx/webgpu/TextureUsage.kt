package androidx.webgpu

@JvmInline
public value class TextureUsage(public val value: Int) {
    public infix fun or(b: TextureUsage): TextureUsage = TextureUsage(this.value or b.value)
    public companion object {
        public val None: TextureUsage = TextureUsage(0x00000000)
        public val CopySrc: TextureUsage = TextureUsage(0x00000001)
        public val CopyDst: TextureUsage = TextureUsage(0x00000002)
        public val TextureBinding: TextureUsage = TextureUsage(0x00000004)
        public val StorageBinding: TextureUsage = TextureUsage(0x00000008)
        public val RenderAttachment: TextureUsage = TextureUsage(0x00000010)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "None",
            0x00000001 to "CopySrc",
            0x00000002 to "CopyDst",
            0x00000004 to "TextureBinding",
            0x00000008 to "StorageBinding",
            0x00000010 to "RenderAttachment",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
