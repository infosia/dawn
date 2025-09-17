package androidx.webgpu

public class SurfaceColorManagement(
    @get:JvmName("getColorSpace")
    public var colorSpace: PredefinedColorSpace,
    @get:JvmName("getToneMappingMode")
    public var toneMappingMode: ToneMappingMode,
)
