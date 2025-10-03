package androidx.webgpu

@JvmInline
public value class StencilOperation(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: StencilOperation = StencilOperation(0x00000000)
        public val Keep: StencilOperation = StencilOperation(0x00000001)
        public val Zero: StencilOperation = StencilOperation(0x00000002)
        public val Replace: StencilOperation = StencilOperation(0x00000003)
        public val Invert: StencilOperation = StencilOperation(0x00000004)
        public val IncrementClamp: StencilOperation = StencilOperation(0x00000005)
        public val DecrementClamp: StencilOperation = StencilOperation(0x00000006)
        public val IncrementWrap: StencilOperation = StencilOperation(0x00000007)
        public val DecrementWrap: StencilOperation = StencilOperation(0x00000008)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Keep",
            0x00000002 to "Zero",
            0x00000003 to "Replace",
            0x00000004 to "Invert",
            0x00000005 to "IncrementClamp",
            0x00000006 to "DecrementClamp",
            0x00000007 to "IncrementWrap",
            0x00000008 to "DecrementWrap",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
