package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


/**
 * An object used to continuously present image data to the user, see @ref Surfaces
 * for more details.
 */
public class Surface(public val handle: Long): AutoCloseable {
    /**
    * Configures parameters for rendering to `surface`. Produces a @ref DeviceError
    * for all content-timeline errors defined by the WebGPU specification.  See @ref
    * Surface-Configuration for more details.
    * @param configThe new configuration to use.    
    */
    @FastNative
    @JvmName("configure")
    public external fun configure(config: SurfaceConfiguration, ): Unit

    /**
    * Provides information on how `adapter` is able to use `surface`. See @ref
    * Surface-Capabilities for more details.
    * @param adapterThe @ref WGPUAdapter to get capabilities for presenting to this @ref WGPUSurface.
    * @return Indicates if there was an @ref OutStructChainError.    
    */
    @FastNative
    @JvmName("getCapabilities")
    public external fun getCapabilities(adapter: Adapter, ): SurfaceCapabilities

    /**
    * Returns the @ref WGPUTexture to render to `surface` this frame along with
    * metadata on the frame. Returns `NULL` and @ref
    * WGPUSurfaceGetCurrentTextureStatus_Error if the surface is not configured.  See
    * @ref Surface-Presenting for more details.    
    */
    @FastNative
    @JvmName("getCurrentTexture")
    public external fun getCurrentTexture(): SurfaceTexture

    /**
    * Shows `surface`'s current texture to the user. See @ref Surface-Presenting for
    * more details.
    * @return Returns @ref WGPUStatus_Error if the surface doesn't have a current texture.    
    */
    @FastNative
    @JvmName("present")
    public external fun present(): Status

    /**
    * Modifies the label used to refer to `surface`.
    * @param labelThe new label.    
    */
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    /**
    * Removes the configuration for `surface`. See @ref Surface-Configuration for more
    * details.    
    */
    @FastNative
    @JvmName("unconfigure")
    public external fun unconfigure(): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Surface && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
