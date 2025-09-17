package androidx.webgpu

public class TexelCopyBufferLayout(
    public var offset: Long = 0,
    public var bytesPerRow: Int = Constants.COPY_STRIDE_UNDEFINED,
    public var rowsPerImage: Int = Constants.COPY_STRIDE_UNDEFINED,
)
