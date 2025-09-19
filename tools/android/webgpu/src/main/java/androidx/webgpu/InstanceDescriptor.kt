package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class InstanceDescriptor(
    @get:JvmName("getRequiredFeatures")
    public var requiredFeatures: Array<InstanceFeatureName> = arrayOf(),
    public var requiredLimits: InstanceLimits? = InstanceLimits(),
)
