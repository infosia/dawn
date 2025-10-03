package androidx.webgpu

@JvmInline
public value class SurfaceGetCurrentTextureStatus(public val value: Int) {
    public companion object {
        /**
         * Yay! Everything is good and we can render this frame.
         */
        public val SuccessOptimal: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000001)
        /**
         * Still OK - the surface can present the frame, but in a suboptimal way. The
         * surface may need reconfiguration.
         */
        public val SuccessSuboptimal: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000002)
        /**
         * Some operation timed out while trying to acquire the frame.
         */
        public val Timeout: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000003)
        /**
         * The surface is too different to be used, compared to when it was originally
         * created.
         */
        public val Outdated: SurfaceGetCurrentTextureStatus = SurfaceGetCurrentTextureStatus(0x00000004)
        /**
         * The connection to whatever owns the surface was lost, or generally needs to be
         * fully reinitialized.
         */
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
