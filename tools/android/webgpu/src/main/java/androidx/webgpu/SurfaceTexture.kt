package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SurfaceTexture(
    public var texture: Texture,
    @get:JvmName("getStatus")
    public var status: SurfaceGetCurrentTextureStatus,
)
