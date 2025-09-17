package androidx.webgpu

@JvmInline
public value class DeviceLostReason(public val value: Int) {
    public companion object {
        public val Unknown: DeviceLostReason = DeviceLostReason(0x00000001)
        public val Destroyed: DeviceLostReason = DeviceLostReason(0x00000002)
        public val CallbackCancelled: DeviceLostReason = DeviceLostReason(0x00000003)
        public val FailedCreation: DeviceLostReason = DeviceLostReason(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Unknown",
            0x00000002 to "Destroyed",
            0x00000003 to "CallbackCancelled",
            0x00000004 to "FailedCreation",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
