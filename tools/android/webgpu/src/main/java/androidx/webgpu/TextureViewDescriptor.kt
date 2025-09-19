package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class TextureViewDescriptor(
    public var label: String? = null,
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    @get:JvmName("getDimension")
    public var dimension: TextureViewDimension = TextureViewDimension.Undefined,
    public var baseMipLevel: Int = 0,
    public var mipLevelCount: Int = Constants.MIP_LEVEL_COUNT_UNDEFINED,
    public var baseArrayLayer: Int = 0,
    public var arrayLayerCount: Int = Constants.ARRAY_LAYER_COUNT_UNDEFINED,
    /**
     * If set to @see TextureAspect.Undefined, defaults to @see TextureAspect.All.
     */
    @get:JvmName("getAspect")
    public var aspect: TextureAspect = TextureAspect.All,
    @get:JvmName("getUsage")
    public var usage: TextureUsage,
)
