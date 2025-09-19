package androidx.webgpu

/**
 * The root descriptor for the creation of an @ref WGPUSurface with @ref
 * wgpuInstanceCreateSurface. It isn't sufficient by itself and must have one of
 * the `WGPUSurfaceSource*` in its chain. See @ref Surface-Creation for more
 * details.
 */
public class SurfaceDescriptor(
    /**
     * Label used to refer to the object.
     */
    public var label: String? = null,
    /**
     * Extension of @ref WGPUSurfaceConfiguration for color spaces and HDR.
     */
    public var surfaceColorManagement: SurfaceColorManagement? = null,
    /**
     * Chained in @ref WGPUSurfaceDescriptor to make an @ref WGPUSurface wrapping an
     * Android
     * [`ANativeWindow`](https://developer.android.com/ndk/reference/group/a-native-window).
     */
    public var surfaceSourceAndroidNativeWindow: SurfaceSourceAndroidNativeWindow? = null,
)
