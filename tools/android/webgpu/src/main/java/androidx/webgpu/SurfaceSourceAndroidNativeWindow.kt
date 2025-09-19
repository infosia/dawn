package androidx.webgpu

/**
 * Chained in @ref WGPUSurfaceDescriptor to make an @ref WGPUSurface wrapping an
 * Android
 * [`ANativeWindow`](https://developer.android.com/ndk/reference/group/a-native-window).
 */
public class SurfaceSourceAndroidNativeWindow(
    /**
     * The pointer to the
     * [`ANativeWindow`](https://developer.android.com/ndk/reference/group/a-native-window)
     * that will be wrapped by the @ref WGPUSurface.
     */
    public var window: Long
,
)
