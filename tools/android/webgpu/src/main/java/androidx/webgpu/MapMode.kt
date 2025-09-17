package androidx.webgpu

@JvmInline
public value class MapMode(public val value: Int) {
    public infix fun or(b: MapMode): MapMode = MapMode(this.value or b.value)
    public companion object {
        public val None: MapMode = MapMode(0x00000000)
        public val Read: MapMode = MapMode(0x00000001)
        public val Write: MapMode = MapMode(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "None",
            0x00000001 to "Read",
            0x00000002 to "Write",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
