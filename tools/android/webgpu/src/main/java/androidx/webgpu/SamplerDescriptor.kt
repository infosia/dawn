package androidx.webgpu

public class SamplerDescriptor(
    public var label: String? = null,
    @get:JvmName("getAddressModeU")
    public var addressModeU: AddressMode = AddressMode.ClampToEdge,
    @get:JvmName("getAddressModeV")
    public var addressModeV: AddressMode = AddressMode.ClampToEdge,
    @get:JvmName("getAddressModeW")
    public var addressModeW: AddressMode = AddressMode.ClampToEdge,
    @get:JvmName("getMagFilter")
    public var magFilter: FilterMode = FilterMode.Nearest,
    @get:JvmName("getMinFilter")
    public var minFilter: FilterMode = FilterMode.Nearest,
    @get:JvmName("getMipmapFilter")
    public var mipmapFilter: MipmapFilterMode = MipmapFilterMode.Nearest,
    public var lodMinClamp: Float = 0.0f,
    public var lodMaxClamp: Float = 32.0f,
    @get:JvmName("getCompare")
    public var compare: CompareFunction = CompareFunction.Undefined,
    public var maxAnisotropy: Short = 1,
)
