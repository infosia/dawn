package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Adapter(public val handle: Long): AutoCloseable {
    /**
    * Get the list of @ref WGPUFeatureName values supported by the adapter.
    */
    @FastNative
    @JvmName("getFeatures")
    public external fun getFeatures(): SupportedFeatures

    /**
    * @return Indicates if there was an @ref OutStructChainError.
    */
    @FastNative
    @JvmName("getInfo")
    public external fun getInfo(): AdapterInfo

    /**
    * @return Indicates if there was an @ref OutStructChainError.
    */
    @FastNative
    @JvmName("getLimits")
    public external fun getLimits(): Limits

    @FastNative
    @JvmName("hasFeature")
    public external fun hasFeature(feature: FeatureName, ): Boolean

    @FastNative
    @JvmName("requestDevice")
    public external fun requestDevice(descriptor: DeviceDescriptor? = null, callbackInfo: RequestDeviceCallbackInfo, ): Future

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Adapter && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
