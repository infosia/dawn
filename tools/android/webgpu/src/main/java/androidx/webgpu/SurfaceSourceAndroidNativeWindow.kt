package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class SurfaceSourceAndroidNativeWindow(
    /**
     * The pointer to the
     * [`ANativeWindow`](https://developer.android.com/ndk/reference/group/a-native-window)
     * that will be wrapped by the @see Surface.
     */
    public var window: Long
,
)
