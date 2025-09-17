package androidx.webgpu

public class CompilationMessage(
    public var message: String,
    @get:JvmName("getType")
    public var type: CompilationMessageType,
    public var lineNum: Long,
    public var linePos: Long,
    public var offset: Long,
    public var length: Long,
)
