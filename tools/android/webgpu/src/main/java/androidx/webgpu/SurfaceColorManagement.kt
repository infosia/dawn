package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Extension of @see SurfaceConfiguration for color spaces and HDR.
 */
public class SurfaceColorManagement(
    @get:JvmName("getColorSpace")
    public var colorSpace: PredefinedColorSpace,
    @get:JvmName("getToneMappingMode")
    public var toneMappingMode: ToneMappingMode,
)
