package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class ShaderModule(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("getCompilationInfo")
    public external fun getCompilationInfo(callbackInfo: CompilationInfoCallbackInfo, ): Future

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is ShaderModule && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
