package androidx.webgpu

public class SamplerBindingLayout(
    /**
     * If set to @see SamplerBindingType.Undefined, defaults to @see
     * SamplerBindingType.Filtering.
     */
    @get:JvmName("getType")
    public var type: SamplerBindingType = SamplerBindingType.Filtering,
)
