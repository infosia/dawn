package androidx.webgpu

@JvmInline
public value class BlendOperation(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: BlendOperation = BlendOperation(0x00000000)
        public val Add: BlendOperation = BlendOperation(0x00000001)
        public val Subtract: BlendOperation = BlendOperation(0x00000002)
        public val ReverseSubtract: BlendOperation = BlendOperation(0x00000003)
        public val Min: BlendOperation = BlendOperation(0x00000004)
        public val Max: BlendOperation = BlendOperation(0x00000005)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Add",
            0x00000002 to "Subtract",
            0x00000003 to "ReverseSubtract",
            0x00000004 to "Min",
            0x00000005 to "Max",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
