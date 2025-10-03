package androidx.webgpu

@JvmInline
public value class ShaderStage(public val value: Int) {
    public infix fun or(b: ShaderStage): ShaderStage = ShaderStage(this.value or b.value)
    public companion object {
        public val None: ShaderStage = ShaderStage(0x00000000)
        public val Vertex: ShaderStage = ShaderStage(0x00000001)
        public val Fragment: ShaderStage = ShaderStage(0x00000002)
        public val Compute: ShaderStage = ShaderStage(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "None",
            0x00000001 to "Vertex",
            0x00000002 to "Fragment",
            0x00000004 to "Compute",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
