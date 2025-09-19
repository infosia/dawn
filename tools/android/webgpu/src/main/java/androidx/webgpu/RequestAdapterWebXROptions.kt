package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Extension providing requestAdapter options for implementations with WebXR
 * interop (i.e. Wasm).
 */
public class RequestAdapterWebXROptions(
    public var xrCompatible: Boolean,
)
