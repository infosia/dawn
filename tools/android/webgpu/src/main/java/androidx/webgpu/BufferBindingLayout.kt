package androidx.webgpu

public class BufferBindingLayout(
    @get:JvmName("getType")
    public var type: BufferBindingType = BufferBindingType.Uniform,
    public var hasDynamicOffset: Boolean = false,
    public var minBindingSize: Long = 0,
)
