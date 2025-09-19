package androidx.webgpu

public class StorageTextureBindingLayout(
    /**
     * If set to @see StorageTextureAccess.Undefined, defaults to @see
     * StorageTextureAccess.WriteOnly.
     */
    @get:JvmName("getAccess")
    public var access: StorageTextureAccess = StorageTextureAccess.WriteOnly,
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    @get:JvmName("getViewDimension")
    public var viewDimension: TextureViewDimension = TextureViewDimension._2D,
)
