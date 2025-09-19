package androidx.webgpu

/**
 * Extension providing requestAdapter options for implementations with WebXR
 * interop (i.e. Wasm).
 */
public class RequestAdapterWebXROptions(
    public var xrCompatible: Boolean,
)
