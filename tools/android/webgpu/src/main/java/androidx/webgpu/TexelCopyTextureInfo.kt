package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class TexelCopyTextureInfo(
    public var texture: Texture,
    public var mipLevel: Int = 0,
    public var origin: Origin3D = Origin3D(),
    /**
     * If set to @see TextureAspect.Undefined, defaults to @see TextureAspect.All.
     */
    @get:JvmName("getAspect")
    public var aspect: TextureAspect = TextureAspect.All,
)
