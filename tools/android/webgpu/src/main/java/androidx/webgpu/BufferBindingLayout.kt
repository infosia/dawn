package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BufferBindingLayout(
    /**
     * If set to @ref WGPUBufferBindingType_Undefined, [defaults](@ref SentinelValues)
     * to @ref WGPUBufferBindingType_Uniform.
     */
    @get:JvmName("getType")
    public var type: BufferBindingType = BufferBindingType.Uniform,
    public var hasDynamicOffset: Boolean = false,
    public var minBindingSize: Long = 0,
)
