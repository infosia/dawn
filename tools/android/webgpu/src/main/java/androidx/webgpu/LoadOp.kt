package androidx.webgpu

@JvmInline
public value class LoadOp(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: LoadOp = LoadOp(0x00000000)
        public val Load: LoadOp = LoadOp(0x00000001)
        public val Clear: LoadOp = LoadOp(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Load",
            0x00000002 to "Clear",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
