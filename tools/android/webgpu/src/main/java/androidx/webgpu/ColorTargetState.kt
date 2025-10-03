package androidx.webgpu

public class ColorTargetState(
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    public var blend: BlendState? = null,
    @get:JvmName("getWriteMask")
    public var writeMask: ColorWriteMask = ColorWriteMask.All,
)
