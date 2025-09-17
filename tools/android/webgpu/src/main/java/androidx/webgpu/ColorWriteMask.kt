package androidx.webgpu

@JvmInline
public value class ColorWriteMask(public val value: Int) {
    public infix fun or(b: ColorWriteMask): ColorWriteMask = ColorWriteMask(this.value or b.value)
    public companion object {
        public val None: ColorWriteMask = ColorWriteMask(0x00000000)
        public val Red: ColorWriteMask = ColorWriteMask(0x00000001)
        public val Green: ColorWriteMask = ColorWriteMask(0x00000002)
        public val Blue: ColorWriteMask = ColorWriteMask(0x00000004)
        public val Alpha: ColorWriteMask = ColorWriteMask(0x00000008)
        public val All: ColorWriteMask = ColorWriteMask(0x0000000f)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "None",
            0x00000001 to "Red",
            0x00000002 to "Green",
            0x00000004 to "Blue",
            0x00000008 to "Alpha",
            0x0000000f to "All",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
