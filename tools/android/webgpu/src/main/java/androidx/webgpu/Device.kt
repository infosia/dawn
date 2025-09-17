package androidx.webgpu

import dalvik.annotation.optimization.FastNative
import java.nio.ByteBuffer


public class Device(public val handle: Long): AutoCloseable {
    @FastNative
    @JvmName("createBindGroup")
    public external fun createBindGroup(descriptor: BindGroupDescriptor, ): BindGroup

    @FastNative
    @JvmName("createBindGroupLayout")
    public external fun createBindGroupLayout(descriptor: BindGroupLayoutDescriptor, ): BindGroupLayout

    @FastNative
    @JvmName("createBuffer")
    public external fun createBuffer(descriptor: BufferDescriptor, ): Buffer

    @FastNative
    @JvmName("createCommandEncoder")
    public external fun createCommandEncoder(descriptor: CommandEncoderDescriptor? = CommandEncoderDescriptor(), ): CommandEncoder

    @FastNative
    @JvmName("createComputePipeline")
    public external fun createComputePipeline(descriptor: ComputePipelineDescriptor, ): ComputePipeline

    @FastNative
    @JvmName("createComputePipelineAsync")
    public external fun createComputePipelineAsync(descriptor: ComputePipelineDescriptor, callbackInfo: CreateComputePipelineAsyncCallbackInfo, ): Future

    @FastNative
    @JvmName("createPipelineLayout")
    public external fun createPipelineLayout(descriptor: PipelineLayoutDescriptor, ): PipelineLayout

    @FastNative
    @JvmName("createQuerySet")
    public external fun createQuerySet(descriptor: QuerySetDescriptor, ): QuerySet

    @FastNative
    @JvmName("createRenderBundleEncoder")
    public external fun createRenderBundleEncoder(descriptor: RenderBundleEncoderDescriptor, ): RenderBundleEncoder

    @FastNative
    @JvmName("createRenderPipeline")
    public external fun createRenderPipeline(descriptor: RenderPipelineDescriptor, ): RenderPipeline

    @FastNative
    @JvmName("createRenderPipelineAsync")
    public external fun createRenderPipelineAsync(descriptor: RenderPipelineDescriptor, callbackInfo: CreateRenderPipelineAsyncCallbackInfo, ): Future

    @FastNative
    @JvmName("createSampler")
    public external fun createSampler(descriptor: SamplerDescriptor? = SamplerDescriptor(), ): Sampler

    @FastNative
    @JvmName("createShaderModule")
    public external fun createShaderModule(descriptor: ShaderModuleDescriptor = ShaderModuleDescriptor(), ): ShaderModule

    @FastNative
    @JvmName("createTexture")
    public external fun createTexture(descriptor: TextureDescriptor, ): Texture

    @FastNative
    @JvmName("destroy")
    public external fun destroy(): Unit

    @FastNative
    @JvmName("getAdapterInfo")
    public external fun getAdapterInfo(): AdapterInfo

    @FastNative
    @JvmName("getFeatures")
    public external fun getFeatures(): SupportedFeatures

    @FastNative
    @JvmName("getLimits")
    public external fun getLimits(): Limits

    @FastNative
    @JvmName("getLostFuture")
    public external fun getLostFuture(): Future

    @get:JvmName("lostFuture")
    public val lostFuture: Future get() = getLostFuture()

    @FastNative
    @JvmName("getQueue")
    public external fun getQueue(): Queue

    @get:JvmName("queue")
    public val queue: Queue get() = getQueue()

    @FastNative
    @JvmName("hasFeature")
    public external fun hasFeature(feature: FeatureName, ): Boolean

    @FastNative
    @JvmName("popErrorScope")
    public external fun popErrorScope(callbackInfo: PopErrorScopeCallbackInfo, ): Future

    @FastNative
    @JvmName("pushErrorScope")
    public external fun pushErrorScope(filter: ErrorFilter, ): Unit

    @FastNative
    @JvmName("setLabel")
    public external fun setLabel(label: String, ): Unit

    external override fun close();

    override fun equals(other: Any?): Boolean =
        other is Device && other.handle == handle
    override fun hashCode(): Int = handle.hashCode()
}
