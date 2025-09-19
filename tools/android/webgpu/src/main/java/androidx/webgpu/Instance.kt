package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Instance(public val handle: Long): AutoCloseable {
    /**
     * @param descriptor The description of the @see Surface to create.
     * @return A new @see Surface for this descriptor (or an error @see Surface).
     */
    @FastNative
    @JvmName("createSurface")
    public external fun createSurface(descriptor: SurfaceDescriptor = SurfaceDescriptor(), ): Surface

    /**
     * Get the list of @see WGSLLanguageFeatureName values supported by the instance.
     */
    @FastNative
    @JvmName("getWGSLLanguageFeatures")
    public external fun getWGSLLanguageFeatures(): SupportedWGSLLanguageFeatures

    @FastNative
    @JvmName("hasWGSLLanguageFeature")
    public external fun hasWGSLLanguageFeature(feature: WGSLLanguageFeatureName, ): Boolean

    @FastNative
    @JvmName("processEvents")
    public external fun processEvents(): Unit

    @FastNative
    @JvmName("requestAdapter")
    public external fun requestAdapter(options: RequestAdapterOptions? = RequestAdapterOptions(), callbackInfo: RequestAdapterCallbackInfo, ): Future

    @FastNative
    @JvmName("waitAny")
    public external fun waitAny(futures: Array<FutureWaitInfo> = arrayOf(), timeoutNS: Long, ): WaitStatus

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Instance && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
