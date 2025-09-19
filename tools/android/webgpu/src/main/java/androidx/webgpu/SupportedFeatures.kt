package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SupportedFeatures(
    @get:JvmName("getFeatures")
    public var features: Array<FeatureName> = arrayOf(),
)
