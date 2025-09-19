package androidx.webgpu

public class BufferBindingLayout(
    /**
     * If set to @see BufferBindingType.Undefined, defaults to @see
     * BufferBindingType.Uniform .
     */
    @get:JvmName("getType")
    public var type: BufferBindingType = BufferBindingType.Uniform,
    public var hasDynamicOffset: Boolean = false,
    public var minBindingSize: Long = 0,
)
