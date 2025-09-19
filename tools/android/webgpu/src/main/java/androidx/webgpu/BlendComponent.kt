package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BlendComponent(
    /**
     * If set to @see BlendOperation.Undefined, defaults to @see BlendOperation.Add.
     */
    @get:JvmName("getOperation")
    public var operation: BlendOperation = BlendOperation.Add,
    @get:JvmName("getSrcFactor")
    public var srcFactor: BlendFactor = BlendFactor.One,
    @get:JvmName("getDstFactor")
    public var dstFactor: BlendFactor = BlendFactor.Zero,
)
