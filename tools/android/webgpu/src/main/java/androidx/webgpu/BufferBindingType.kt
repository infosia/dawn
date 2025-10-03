package androidx.webgpu

@JvmInline
public value class BufferBindingType(public val value: Int) {
    public companion object {
        /**
         * Indicates that this @see BufferBindingLayout member of its parent @see
         * BindGroupLayoutEntry is not used.
         */
        public val BindingNotUsed: BufferBindingType = BufferBindingType(0x00000000)
        /**
         * `1`. Indicates no value is passed for this argument.
         */
        public val Undefined: BufferBindingType = BufferBindingType(0x00000001)
        public val Uniform: BufferBindingType = BufferBindingType(0x00000002)
        public val Storage: BufferBindingType = BufferBindingType(0x00000003)
        public val ReadOnlyStorage: BufferBindingType = BufferBindingType(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "BindingNotUsed",
            0x00000001 to "Undefined",
            0x00000002 to "Uniform",
            0x00000003 to "Storage",
            0x00000004 to "ReadOnlyStorage",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
