package androidx.webgpu

public class MultisampleState(
    public var count: Int = 1,
    public var mask: Int = -0x7FFFFFFF,
    public var alphaToCoverageEnabled: Boolean = false,
)
