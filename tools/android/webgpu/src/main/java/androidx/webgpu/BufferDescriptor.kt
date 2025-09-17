package androidx.webgpu

public class BufferDescriptor(
    public var label: String? = null,
    @get:JvmName("getUsage")
    public var usage: BufferUsage,
    public var size: Long,
    public var mappedAtCreation: Boolean = false,
)
