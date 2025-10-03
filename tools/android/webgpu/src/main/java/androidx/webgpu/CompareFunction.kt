package androidx.webgpu

@JvmInline
public value class CompareFunction(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: CompareFunction = CompareFunction(0x00000000)
        public val Never: CompareFunction = CompareFunction(0x00000001)
        public val Less: CompareFunction = CompareFunction(0x00000002)
        public val Equal: CompareFunction = CompareFunction(0x00000003)
        public val LessEqual: CompareFunction = CompareFunction(0x00000004)
        public val Greater: CompareFunction = CompareFunction(0x00000005)
        public val NotEqual: CompareFunction = CompareFunction(0x00000006)
        public val GreaterEqual: CompareFunction = CompareFunction(0x00000007)
        public val Always: CompareFunction = CompareFunction(0x00000008)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Never",
            0x00000002 to "Less",
            0x00000003 to "Equal",
            0x00000004 to "LessEqual",
            0x00000005 to "Greater",
            0x00000006 to "NotEqual",
            0x00000007 to "GreaterEqual",
            0x00000008 to "Always",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
