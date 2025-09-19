package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * Queried each frame from a @ref WGPUSurface to get a @ref WGPUTexture to render
 * to along with some metadata. See @ref Surface-Presenting for more details.
 */
public class SurfaceTexture(
    /**
     * The @ref WGPUTexture representing the frame that will be shown on the surface.
     * It is @ref ReturnedWithOwnership from @ref wgpuSurfaceGetCurrentTexture.
     */
    public var texture: Texture,
    /**
     * Whether the call to @ref wgpuSurfaceGetCurrentTexture succeeded and a hint as to
     * why it might not have.
     */
    @get:JvmName("getStatus")
    public var status: SurfaceGetCurrentTextureStatus,
)
