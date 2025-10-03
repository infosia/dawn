package androidx.webgpu

@JvmInline
public value class PresentMode(public val value: Int) {
    public companion object {
        /**
         * Present mode is not specified. Use the default.
         */
        public val Undefined: PresentMode = PresentMode(0x00000000)
        /**
         * The presentation of the image to the user waits for the next vertical blanking
         * period to update in a first-in, first-out manner. Tearing cannot be observed and
         * frame-loop will be limited to the display's refresh rate. This is the only mode
         * that's always available.
         */
        public val Fifo: PresentMode = PresentMode(0x00000001)
        /**
         * The presentation of the image to the user tries to wait for the next vertical
         * blanking period but may decide to not wait if a frame is presented late. Tearing
         * can sometimes be observed but late-frame don't produce a full-frame stutter in
         * the presentation. This is still a first-in, first-out mechanism so a frame-loop
         * will be limited to the display's refresh rate.
         */
        public val FifoRelaxed: PresentMode = PresentMode(0x00000002)
        /**
         * The presentation of the image to the user is updated immediately without waiting
         * for a vertical blank. Tearing can be observed but latency is minimized.
         */
        public val Immediate: PresentMode = PresentMode(0x00000003)
        /**
         * The presentation of the image to the user waits for the next vertical blanking
         * period to update to the latest provided image. Tearing cannot be observed and a
         * frame-loop is not limited to the display's refresh rate.
         */
        public val Mailbox: PresentMode = PresentMode(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Undefined",
            0x00000001 to "Fifo",
            0x00000002 to "FifoRelaxed",
            0x00000003 to "Immediate",
            0x00000004 to "Mailbox",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
