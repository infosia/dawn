package androidx.webgpu

@JvmInline
public value class InstanceFeatureName(public val value: Int) {
    public companion object {
        public val TimedWaitAny: InstanceFeatureName = InstanceFeatureName(0x00000001)
        public val ShaderSourceSPIRV: InstanceFeatureName = InstanceFeatureName(0x00000002)
        public val MultipleDevicesPerAdapter: InstanceFeatureName = InstanceFeatureName(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "TimedWaitAny",
            0x00000002 to "ShaderSourceSPIRV",
            0x00000003 to "MultipleDevicesPerAdapter",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
