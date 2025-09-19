package androidx.webgpu

public class ColorTargetState(
    /**
     * The texture format of the target. If @see TextureFormat.Undefined, indicates a
     * "hole" in the parent @see FragmentState `targets` array: the pipeline does not
     * output a value at this `location`.
     */
    @get:JvmName("getFormat")
    public var format: TextureFormat = TextureFormat.Undefined,
    public var blend: BlendState? = null,
    @get:JvmName("getWriteMask")
    public var writeMask: ColorWriteMask = ColorWriteMask.All,
)
