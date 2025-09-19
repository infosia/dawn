package androidx.webgpu

@JvmInline
public value class VertexStepMode(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: VertexStepMode = VertexStepMode(0x00000000)
        public val Vertex: VertexStepMode = VertexStepMode(0x00000001)
        public val Instance: VertexStepMode = VertexStepMode(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Vertex",
            0x00000002 to "Instance",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
