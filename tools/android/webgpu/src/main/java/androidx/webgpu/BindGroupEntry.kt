package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class BindGroupEntry(
    /**
     * Binding index in the bind group.
     */
    public var binding: Int,
    /**
     * Set this if the binding is a buffer object. Otherwise must be null.
     */
    public var buffer: Buffer? = null,
    /**
     * If the binding is a buffer, this is the byte offset of the binding range.
     * Otherwise ignored.
     */
    public var offset: Long = 0,
    /**
     * If the binding is a buffer, this is the byte size of the binding range (@see
     * Constants.WHOLE_SIZE means the binding ends at the end of the buffer). Otherwise
     * ignored.
     */
    public var size: Long = Constants.WHOLE_SIZE,
    /**
     * Set this if the binding is a sampler object. Otherwise must be null.
     */
    public var sampler: Sampler? = null,
    public var textureView: TextureView? = null,
)
