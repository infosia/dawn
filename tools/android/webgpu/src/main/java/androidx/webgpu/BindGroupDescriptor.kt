package androidx.webgpu

public class BindGroupDescriptor(
    public var label: String? = null,
    public var layout: BindGroupLayout,
    public var entries: Array<BindGroupEntry> = arrayOf(),
)
