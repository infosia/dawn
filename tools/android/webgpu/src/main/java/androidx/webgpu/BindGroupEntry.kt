package androidx.webgpu

public class BindGroupEntry(
    public var binding: Int,
    public var buffer: Buffer? = null,
    public var offset: Long = 0,
    public var size: Long = Constants.WHOLE_SIZE,
    public var sampler: Sampler? = null,
    public var textureView: TextureView? = null,
)
