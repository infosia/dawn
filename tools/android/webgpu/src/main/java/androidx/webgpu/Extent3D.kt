package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class Extent3D(
    public var width: Int,
    public var height: Int = 1,
    public var depthOrArrayLayers: Int = 1,
)
