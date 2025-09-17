package androidx.webgpu

public class SurfaceDescriptor(
    public var label: String? = null,
    public var surfaceColorManagement: SurfaceColorManagement? = null,
    public var surfaceSourceAndroidNativeWindow: SurfaceSourceAndroidNativeWindow? = null,
)
