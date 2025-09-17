package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Surface(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("configure")
    public external fun configure(config: SurfaceConfiguration, ): Unit

    @FastNative
    @JvmName("getCapabilities")
    public external fun getCapabilities(adapter: Adapter, ): SurfaceCapabilities

    @FastNative
    @JvmName("getCurrentTexture")
    public external fun getCurrentTexture(): SurfaceTexture

    @FastNative
    @JvmName("present")
    public external fun present(): Status

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    @FastNative
    @JvmName("unconfigure")
    public external fun unconfigure(): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Surface && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
