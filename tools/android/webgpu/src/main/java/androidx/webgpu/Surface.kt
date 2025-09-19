package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


/**
 * An object used to continuously present image data to the user, @see Surfaces for
 * more details.
 */
public class Surface(public val handle: Long): AutoCloseable {
    /**
     * Configures parameters for rendering to `surface`. Produces a @see DeviceError
     * for all content-timeline errors defined by the WebGPU specification.  @see
     * SurfaceConfiguration for more details.
     * @param config The new configuration to use.
     */
    @FastNative
    @JvmName("configure")
    public external fun configure(config: SurfaceConfiguration, ): Unit

    /**
     * Provides information on how `adapter` is able to use `surface`. @see
     * SurfaceCapabilities for more details.
     * @param adapter The @see Adapter to get capabilities for presenting to this @see Surface.
     */
    @FastNative
    @JvmName("getCapabilities")
    public external fun getCapabilities(adapter: Adapter, ): SurfaceCapabilities

    @FastNative
    @JvmName("getCurrentTexture")
    public external fun getCurrentTexture(): SurfaceTexture

    /**
     * @return Returns @see Status.Error if the surface doesn't have a current texture.
     */
    @FastNative
    @JvmName("present")
    public external fun present(): Status

    /**
     * Modifies the label used to refer to `surface`.
     * @param label The new label.
     */
    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    /**
     * Removes the configuration for `surface`. @see SurfaceConfiguration for more
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
