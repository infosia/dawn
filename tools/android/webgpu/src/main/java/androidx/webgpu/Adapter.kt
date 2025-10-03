package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Adapter(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("getFeatures")
    public external fun getFeatures(): SupportedFeatures

    @FastNative
    @JvmName("getInfo")
    public external fun getInfo(): AdapterInfo

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
