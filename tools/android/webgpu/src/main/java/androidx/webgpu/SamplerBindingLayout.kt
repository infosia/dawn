package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SamplerBindingLayout(
    /**
     * If set to @ref WGPUSamplerBindingType_Undefined, [defaults](@ref SentinelValues)
     * to @ref WGPUSamplerBindingType_Filtering.
     */
    @get:JvmName("getType")
    public var type: SamplerBindingType = SamplerBindingType.Filtering,
)
