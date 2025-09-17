package androidx.webgpu

public class VertexAttribute(
    @get:JvmName("getFormat")
    public var format: VertexFormat,
    public var offset: Long,
    public var shaderLocation: Int,
)
