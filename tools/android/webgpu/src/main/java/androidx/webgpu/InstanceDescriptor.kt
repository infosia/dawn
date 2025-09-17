package androidx.webgpu

public class InstanceDescriptor(
    @get:JvmName("getRequiredFeatures")
    public var requiredFeatures: Array<InstanceFeatureName> = arrayOf(),
    public var requiredLimits: InstanceLimits? = InstanceLimits(),
)
