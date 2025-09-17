package androidx.webgpu

@JvmInline
public value class SType(public val value: Int) {
    public companion object {
        public val ShaderSourceSPIRV: SType = SType(0x00000001)
        public val ShaderSourceWGSL: SType = SType(0x00000002)
        public val RenderPassMaxDrawCount: SType = SType(0x00000003)
        public val SurfaceSourceMetalLayer: SType = SType(0x00000004)
        public val SurfaceSourceWindowsHWND: SType = SType(0x00000005)
        public val SurfaceSourceXlibWindow: SType = SType(0x00000006)
        public val SurfaceSourceWaylandSurface: SType = SType(0x00000007)
        public val SurfaceSourceAndroidNativeWindow: SType = SType(0x00000008)
        public val SurfaceSourceXCBWindow: SType = SType(0x00000009)
        public val SurfaceColorManagement: SType = SType(0x0000000a)
        public val RequestAdapterWebXROptions: SType = SType(0x0000000b)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "ShaderSourceSPIRV",
            0x00000002 to "ShaderSourceWGSL",
            0x00000003 to "RenderPassMaxDrawCount",
            0x00000004 to "SurfaceSourceMetalLayer",
            0x00000005 to "SurfaceSourceWindowsHWND",
            0x00000006 to "SurfaceSourceXlibWindow",
            0x00000007 to "SurfaceSourceWaylandSurface",
            0x00000008 to "SurfaceSourceAndroidNativeWindow",
            0x00000009 to "SurfaceSourceXCBWindow",
            0x0000000a to "SurfaceColorManagement",
            0x0000000b to "RequestAdapterWebXROptions",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
