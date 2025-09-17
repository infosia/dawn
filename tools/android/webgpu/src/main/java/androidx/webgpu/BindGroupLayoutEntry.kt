package androidx.webgpu

public class BindGroupLayoutEntry(
    public var binding: Int,
    @get:JvmName("getVisibility")
    public var visibility: ShaderStage,
    public var bindingArraySize: Int = 0,
    public var buffer: BufferBindingLayout = BufferBindingLayout(type =BufferBindingType.BindingNotUsed, ),
    public var sampler: SamplerBindingLayout = SamplerBindingLayout(type =SamplerBindingType.BindingNotUsed, ),
    public var texture: TextureBindingLayout = TextureBindingLayout(sampleType =TextureSampleType.BindingNotUsed, viewDimension =TextureViewDimension.Undefined, ),
    public var storageTexture: StorageTextureBindingLayout = StorageTextureBindingLayout(access =StorageTextureAccess.BindingNotUsed, format =TextureFormat.Undefined, viewDimension =TextureViewDimension.Undefined, ),
)
