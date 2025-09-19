package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BufferDescriptor(
    public var label: String? = null,
    @get:JvmName("getUsage")
    public var usage: BufferUsage,
    public var size: Long,
    public var mappedAtCreation: Boolean = false,
)
