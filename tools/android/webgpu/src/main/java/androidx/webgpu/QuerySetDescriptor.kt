package androidx.webgpu

public class QuerySetDescriptor(
    public var label: String? = null,
    @get:JvmName("getType")
    public var type: QueryType,
    public var count: Int,
)
