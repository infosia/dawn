package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Instance(public val handle: Long): AutoCloseable {
    /**
    * Creates a @ref WGPUSurface, see @ref Surface-Creation for more details.
    * @param descriptor The description of the @ref WGPUSurface to create.
    * @return A new @ref WGPUSurface for this descriptor (or an error @ref WGPUSurface).
    */
    @FastNative
    @JvmName("createSurface")
    public external fun createSurface(descriptor: SurfaceDescriptor = SurfaceDescriptor(), ): Surface

    /**
    * Get the list of @ref WGPUWGSLLanguageFeatureName values supported by the
    * instance.
    */
    @FastNative
    @JvmName("getWGSLLanguageFeatures")
    public external fun getWGSLLanguageFeatures(): SupportedWGSLLanguageFeatures

    @FastNative
    @JvmName("hasWGSLLanguageFeature")
    public external fun hasWGSLLanguageFeature(feature: WGSLLanguageFeatureName, ): Boolean

    /**
    * Processes asynchronous events on this `WGPUInstance`, calling any callbacks for
    * asynchronous operations created with @ref WGPUCallbackMode_AllowProcessEvents.
    * See @ref Process-Events for more information.
    */
    @FastNative
    @JvmName("processEvents")
    public external fun processEvents(): Unit

    @FastNative
    @JvmName("requestAdapter")
    public external fun requestAdapter(options: RequestAdapterOptions? = RequestAdapterOptions(), callbackInfo: RequestAdapterCallbackInfo, ): Future

    /**
    * Wait for at least one WGPUFuture in `futures` to complete, and call callbacks of
    * the respective completed asynchronous operations.  See @ref Wait-Any for more
    * information.
    */
    @FastNative
    @JvmName("waitAny")
    public external fun waitAny(futures: Array<FutureWaitInfo> = arrayOf(), timeoutNS: Long, ): WaitStatus

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Instance && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
