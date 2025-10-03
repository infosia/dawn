package androidx.webgpu

public class RequestAdapterOptions(
    @get:JvmName("getFeatureLevel")
    public var featureLevel: FeatureLevel = FeatureLevel.Core,
    @get:JvmName("getPowerPreference")
    public var powerPreference: PowerPreference = PowerPreference.Undefined,
    public var forceFallbackAdapter: Boolean = false,
    @get:JvmName("getBackendType")
    public var backendType: BackendType = BackendType.Undefined,
    public var compatibleSurface: Surface? = null,
    public var requestAdapterWebXROptions: RequestAdapterWebXROptions? = null,
)
