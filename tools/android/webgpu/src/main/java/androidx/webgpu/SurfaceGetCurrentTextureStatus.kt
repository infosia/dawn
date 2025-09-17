package androidx.webgpu

@JvmInline
public value class SurfaceGetCurrentTextureStatus(public val value: Int) {
    public companion object {
        public val SuccessOptimal: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000001)
        public val SuccessSuboptimal: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000002)
        public val Timeout: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000003)
        public val Outdated: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000004)
        public val Lost: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000005)
        public val Error: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000006)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "SuccessOptimal",
            0x00000002 to "SuccessSuboptimal",
            0x00000003 to "Timeout",
            0x00000004 to "Outdated",
            0x00000005 to "Lost",
            0x00000006 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
