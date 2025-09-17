package androidx.webgpu

public class TexelCopyTextureInfo(
    public var texture: Texture,
    public var mipLevel: Int = 0,
    public var origin: Origin3D = Origin3D(),
    @get:JvmName("getAspect")
    public var aspect: TextureAspect = TextureAspect.All,
)
