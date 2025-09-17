package androidx.webgpu

@JvmInline
public value class BufferUsage(public val value: Int) {
    public infix fun or(b: BufferUsage): BufferUsage = BufferUsage(this.value or b.value)
    public companion object {
        public val None: BufferUsage = BufferUsage(0x00000000)
        public val MapRead: BufferUsage = BufferUsage(0x00000001)
        public val MapWrite: BufferUsage = BufferUsage(0x00000002)
        public val CopySrc: BufferUsage = BufferUsage(0x00000004)
        public val CopyDst: BufferUsage = BufferUsage(0x00000008)
        public val Index: BufferUsage = BufferUsage(0x00000010)
        public val Vertex: BufferUsage = BufferUsage(0x00000020)
        public val Uniform: BufferUsage = BufferUsage(0x00000040)
        public val Storage: BufferUsage = BufferUsage(0x00000080)
        public val Indirect: BufferUsage = BufferUsage(0x00000100)
        public val QueryResolve: BufferUsage = BufferUsage(0x00000200)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "None",
            0x00000001 to "MapRead",
            0x00000002 to "MapWrite",
            0x00000004 to "CopySrc",
            0x00000008 to "CopyDst",
            0x00000010 to "Index",
            0x00000020 to "Vertex",
            0x00000040 to "Uniform",
            0x00000080 to "Storage",
            0x00000100 to "Indirect",
            0x00000200 to "QueryResolve",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
