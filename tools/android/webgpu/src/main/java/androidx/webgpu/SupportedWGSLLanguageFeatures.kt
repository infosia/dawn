package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SupportedWGSLLanguageFeatures(
    @get:JvmName("getFeatures")
    public var features: Array<WGSLLanguageFeatureName> = arrayOf(),
)
