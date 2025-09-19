package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class QuerySetDescriptor(
    public var label: String? = null,
    @get:JvmName("getType")
    public var type: QueryType,
    public var count: Int,
)
