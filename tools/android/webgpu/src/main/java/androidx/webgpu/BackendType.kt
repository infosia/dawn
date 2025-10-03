package androidx.webgpu

@JvmInline
public value class BackendType(public val value: Int) {
    public companion object {
        /**
         * Indicates no value is passed for this argument.
         */
        public val Undefined: BackendType = BackendType(0x00000000)
        public val Null: BackendType = BackendType(0x00000001)
        public val WebGPU: BackendType = BackendType(0x00000002)
        public val D3D11: BackendType = BackendType(0x00000003)
        public val D3D12: BackendType = BackendType(0x00000004)
        public val Metal: BackendType = BackendType(0x00000005)
        public val Vulkan: BackendType = BackendType(0x00000006)
        public val OpenGL: BackendType = BackendType(0x00000007)
        public val OpenGLES: BackendType = BackendType(0x00000008)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Null",
            0x00000002 to "WebGPU",
            0x00000003 to "D3D11",
            0x00000004 to "D3D12",
            0x00000005 to "Metal",
            0x00000006 to "Vulkan",
            0x00000007 to "OpenGL",
            0x00000008 to "OpenGLES",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
