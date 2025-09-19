package androidx.webgpu

public class CompilationMessage(
    public var message: String,
    /**
     * Severity level of the message.
     */
    @get:JvmName("getType")
    public var type: CompilationMessageType,
    public var lineNum: Long,
    public var linePos: Long,
    /**
     * Offset in UTF-8 code units (bytes) from the beginning of the shader code,
     * starting at 0.
     */
    public var offset: Long,
    /**
     * Length in UTF-8 code units (bytes) of the span the message corresponds to.
     */
    public var length: Long,
)
