package androidx.webgpu

@JvmInline
public value class PresentMode(public val value: Int) {
    public companion object {
        public val Undefined: PresentMode = PresentMode(0x00000000)
        public val Fifo: PresentMode = PresentMode(0x00000001)
        public val FifoRelaxed: PresentMode = PresentMode(0x00000002)
        public val Immediate: PresentMode = PresentMode(0x00000003)
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
