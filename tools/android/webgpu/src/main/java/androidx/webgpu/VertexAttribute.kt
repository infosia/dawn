package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class VertexAttribute(
    @get:JvmName("getFormat")
    public var format: VertexFormat,
    public var offset: Long,
    public var shaderLocation: Int,
)
