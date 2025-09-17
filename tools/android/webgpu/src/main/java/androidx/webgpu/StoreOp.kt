package androidx.webgpu

@JvmInline
public value class StoreOp(public val value: Int) {
    public companion object {
        public val Undefined: StoreOp = StoreOp(0x00000000)
        public val Store: StoreOp = StoreOp(0x00000001)
        public val Discard: StoreOp = StoreOp(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Store",
            0x00000002 to "Discard",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
