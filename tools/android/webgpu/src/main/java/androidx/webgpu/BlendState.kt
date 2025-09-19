package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BlendState(
    public var color: BlendComponent = BlendComponent(),
    public var alpha: BlendComponent = BlendComponent(),
)
