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
    /**
     * Extension providing requestAdapter options for implementations with WebXR
     * interop (i.e. Wasm).
     */
    public var requestAdapterWebXROptions: RequestAdapterWebXROptions? = null,
)
