package androidx.webgpu

@JvmInline
public value class BufferUsage(public val value: Int) {
    public infix fun or(b: BufferUsage): BufferUsage = BufferUsage(this.value or b.value)
    public companion object {
        public val None: BufferUsage = BufferUsage(0x00000000)
        /**
         * The buffer can be *mapped* on the CPU side in *read* mode (using @see
         * MapMode.Read).
         */
        public val MapRead: BufferUsage = BufferUsage(0x00000001)
        /**
         * The buffer can be *mapped* on the CPU side in *write* mode (using @see
         * MapMode.Write).  @note This usage is **not** required to set `mappedAtCreation`
         * to `true` in @see BufferDescriptor.
         */
        public val MapWrite: BufferUsage = BufferUsage(0x00000002)
        /**
         * The buffer can be used as the *source* of a GPU-side copy operation.
         */
        public val CopySrc: BufferUsage = BufferUsage(0x00000004)
        /**
         * The buffer can be used as the *destination* of a GPU-side copy operation.
         */
        public val CopyDst: BufferUsage = BufferUsage(0x00000008)
        /**
         * The buffer can be used as an Index buffer when doing indexed drawing in a render
         * pipeline.
         */
        public val Index: BufferUsage = BufferUsage(0x00000010)
        /**
         * The buffer can be used as an Vertex buffer when using a render pipeline.
         */
        public val Vertex: BufferUsage = BufferUsage(0x00000020)
        /**
         * The buffer can be bound to a shader as a uniform buffer.
         */
        public val Uniform: BufferUsage = BufferUsage(0x00000040)
        /**
         * The buffer can be bound to a shader as a storage buffer.
         */
        public val Storage: BufferUsage = BufferUsage(0x00000080)
        /**
         * The buffer can store arguments for an indirect draw call.
         */
        public val Indirect: BufferUsage = BufferUsage(0x00000100)
        /**
         * The buffer can store the result of a timestamp or occlusion query.
         */
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
