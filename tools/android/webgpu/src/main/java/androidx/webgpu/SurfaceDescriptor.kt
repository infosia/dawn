package androidx.webgpu

public class SurfaceDescriptor(
    /**
     * Label used to refer to the object.
     */
    public var label: String? = null,
    /**
     * Extension of @see SurfaceConfiguration for color spaces and HDR.
     */
    public var surfaceColorManagement: SurfaceColorManagement? = null,
    public var surfaceSourceAndroidNativeWindow: SurfaceSourceAndroidNativeWindow? = null,
)
