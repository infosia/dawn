package androidx.webgpu

/**
 * Extension of @see SurfaceConfiguration for color spaces and HDR.
 */
public class SurfaceColorManagement(
    @get:JvmName("getColorSpace")
    public var colorSpace: PredefinedColorSpace,
    @get:JvmName("getToneMappingMode")
    public var toneMappingMode: ToneMappingMode,
)
