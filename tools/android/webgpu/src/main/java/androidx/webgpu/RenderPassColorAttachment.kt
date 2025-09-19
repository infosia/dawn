package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class RenderPassColorAttachment(
    /**
     * If `NULL`, indicates a hole in the parent @see
     * RenderPassDescriptor.colorAttachments array.
     */
    public var view: TextureView? = null,
    public var depthSlice: Int = Constants.DEPTH_SLICE_UNDEFINED,
    public var resolveTarget: TextureView? = null,
    @get:JvmName("getLoadOp")
    public var loadOp: LoadOp = LoadOp.Undefined,
    @get:JvmName("getStoreOp")
    public var storeOp: StoreOp = StoreOp.Undefined,
    public var clearValue: Color,
)
