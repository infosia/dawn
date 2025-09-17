package androidx.webgpu

public class SurfaceTexture(
    public var texture: Texture,
    @get:JvmName("getStatus")
    public var status: SurfaceGetCurrentTextureStatus,
)
