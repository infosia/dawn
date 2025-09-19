package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class PrimitiveState(
    /**
     * If set to @see PrimitiveTopology.Undefined, defaults to @see
     * PrimitiveTopology.TriangleList.
     */
    @get:JvmName("getTopology")
    public var topology: PrimitiveTopology = PrimitiveTopology.TriangleList,
    @get:JvmName("getStripIndexFormat")
    public var stripIndexFormat: IndexFormat = IndexFormat.Undefined,
    @get:JvmName("getFrontFace")
    public var frontFace: FrontFace = FrontFace.CCW,
    @get:JvmName("getCullMode")
    public var cullMode: CullMode = CullMode.None,
    public var unclippedDepth: Boolean = false,
)
