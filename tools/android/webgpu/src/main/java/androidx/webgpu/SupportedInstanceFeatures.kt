package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SupportedInstanceFeatures(
    @get:JvmName("getFeatures")
    public var features: Array<InstanceFeatureName> = arrayOf(),
)
