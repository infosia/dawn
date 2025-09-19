package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class DepthStencilState(
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    @get:JvmName("getDepthWriteEnabled")
    public var depthWriteEnabled: OptionalBool = OptionalBool.Undefined,
    @get:JvmName("getDepthCompare")
    public var depthCompare: CompareFunction = CompareFunction.Undefined,
    public var stencilFront: StencilFaceState = StencilFaceState(),
    public var stencilBack: StencilFaceState = StencilFaceState(),
    public var stencilReadMask: Int = -0x7FFFFFFF,
    public var stencilWriteMask: Int = -0x7FFFFFFF,
    public var depthBias: Int = 0,
    public var depthBiasSlopeScale: Float = 0.0f,
    public var depthBiasClamp: Float = 0.0f,
)
