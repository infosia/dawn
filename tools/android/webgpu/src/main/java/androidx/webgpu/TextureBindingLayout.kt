package androidx.webgpu

public class TextureBindingLayout(
    @get:JvmName("getSampleType")
    public var sampleType: TextureSampleType = TextureSampleType.Float,
    @get:JvmName("getViewDimension")
    public var viewDimension: TextureViewDimension = TextureViewDimension._2D,
    public var multisampled: Boolean = false,
)
