package androidx.webgpu

@JvmInline
public value class InstanceFeatureName(public val value: Int) {
    public companion object {
        /**
         * Enable use of ::wgpuInstanceWaitAny with `timeoutNS > 0`.
         */
        public val TimedWaitAny: InstanceFeatureName = InstanceFeatureName(0x00000001)
        public val ShaderSourceSPIRV: InstanceFeatureName = InstanceFeatureName(0x00000002)
        /**
         * Normally, a @see Adapter can only create a single device. If this is available
         * and enabled, then adapters won't immediately expire when they create a device,
         * so can be reused to make multiple devices. They may still expire for other
         * reasons.
         */
        public val MultipleDevicesPerAdapter: InstanceFeatureName = InstanceFeatureName(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "TimedWaitAny",
            0x00000002 to "ShaderSourceSPIRV",
            0x00000003 to "MultipleDevicesPerAdapter",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
