package androidx.webgpu

public class VertexBufferLayout(
    @get:JvmName("getStepMode")
    public var stepMode: VertexStepMode = VertexStepMode.Undefined,
    public var arrayStride: Long,
    public var attributes: Array<VertexAttribute> = arrayOf(),
)
