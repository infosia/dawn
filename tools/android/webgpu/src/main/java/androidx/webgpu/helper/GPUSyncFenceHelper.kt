package androidx.webgpu.helper

import android.hardware.SyncFence
import android.opengl.EGL14
import android.opengl.EGLConfig
import android.opengl.EGLContext
import android.opengl.EGLDisplay
import android.opengl.EGLExt
import android.opengl.EGLSurface
import android.opengl.EGL15
import android.opengl.EGLSync
import android.os.Build
import androidx.annotation.RequiresApi

/**
 * A thread-safe helper object to convert raw file descriptors to Android `SyncFence`
 * objects using EGL.
 *
 * This object maintains a single persistent EGL context and pbuffer surface to avoid
 * the overhead of creating and destroying them on every conversion.
 * It also ensures that the calling thread's existing EGL state is preserved.
 */
@RequiresApi(Build.VERSION_CODES.TIRAMISU)
internal object GPUSyncFenceHelper {

    private var eglDisplay: EGLDisplay = EGL14.EGL_NO_DISPLAY
    private var eglContext: EGLContext = EGL14.EGL_NO_CONTEXT
    private var eglSurface: EGLSurface = EGL14.EGL_NO_SURFACE

    private val lock = Any()

    /**
     * Ensures that a dedicated EGL context and a pbuffer surface are initialized.
     * This ensures that a current EGL context exists on the calling thread,
     * which is required by some drivers for EGLSync operations.
     */
    private fun ensureEGL() {
        if (eglDisplay != EGL14.EGL_NO_DISPLAY) return
        // 1. Get the default EGL display
        eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY)
        check(eglDisplay != EGL14.EGL_NO_DISPLAY) { "Failed to get EGL display" }

        // 2. Initialize EGL
        val version = IntArray(2)
        check(EGL14.eglInitialize(eglDisplay, version, 0, version, 1)) {
            "Failed to initialize EGL"
        }

        // 3. Choose a configuration that supports Pbuffer and OpenGL ES2
        val configAttribs = intArrayOf(
            EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
            EGL14.EGL_SURFACE_TYPE, EGL14.EGL_PBUFFER_BIT,
            EGL14.EGL_NONE
        )
        val configs = arrayOfNulls<EGLConfig>(1)
        val numConfigs = IntArray(1)
        check(EGL14.eglChooseConfig(eglDisplay, configAttribs, 0, configs, 0, 1, numConfigs, 0)) {
            "Failed to choose EGL config"
        }
        check(numConfigs[0] > 0) { "No EGL config found" }

        // 4. Create an EGL context
        val contextAttribs = intArrayOf(
            EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL14.EGL_NONE
        )
        eglContext = EGL14.eglCreateContext(
            eglDisplay, configs[0], EGL14.EGL_NO_CONTEXT, contextAttribs, 0
        )
        check(eglContext != EGL14.EGL_NO_CONTEXT) { "Failed to create EGL context" }

        // 5. Create a 1x1 Pbuffer surface (needed to make the context current)
        val pbufferAttribs = intArrayOf(
            EGL14.EGL_WIDTH, 1,
            EGL14.EGL_HEIGHT, 1,
            EGL14.EGL_NONE
        )
        eglSurface = EGL14.eglCreatePbufferSurface(eglDisplay, configs[0], pbufferAttribs, 0)
        check(eglSurface != EGL14.EGL_NO_SURFACE) { "Failed to create EGL pbuffer surface" }
    }

    /**
     * Checks if the EGL_ANDROID_native_fence_sync extension is supported.
     */
    internal fun isNativeFenceSyncSupported(): Boolean {
        synchronized(lock) {
            ensureEGL()
            if (eglDisplay == EGL14.EGL_NO_DISPLAY) return false
            val extensions = EGL14.eglQueryString(eglDisplay, EGL14.EGL_EXTENSIONS)
            return extensions?.contains("EGL_ANDROID_native_fence_sync") == true
        }
    }

    /**
     * Creates an Android SyncFence from a raw file descriptor.
     * @param syncFd The raw file descriptor.
     * @return A valid SyncFence, or null if creation failed.
     */
    internal fun createSyncFenceFromFd(syncFd: Int): SyncFence? {
        if (syncFd < 0) return null

        synchronized(lock) {
            ensureEGL()

            if (!isNativeFenceSyncSupported()) {
                 return null
            }

            // Save current EGL state
            val oldDisplay = EGL14.eglGetCurrentDisplay()
            val oldContext = EGL14.eglGetCurrentContext()
            val oldDrawSurface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW)
            val oldReadSurface = EGL14.eglGetCurrentSurface(EGL14.EGL_READ)

            // Make our context current
            check(EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
                "Failed to make EGL context current"
            }

            try {
                // 1. Define attributes to import the file descriptor
                val attributes = longArrayOf(
                    EGL_SYNC_NATIVE_FENCE_FD_ANDROID.toLong(), syncFd.toLong(),
                    EGL14.EGL_NONE.toLong()
                )

                // 2. Create an EGLSync object from the file descriptor
                val eglSync: EGLSync? = EGL15.eglCreateSync(
                    eglDisplay,
                    EGL_SYNC_NATIVE_FENCE_ANDROID,
                    attributes,
                    0
                )
                if (eglSync == null || eglSync == EGL15.EGL_NO_SYNC) {
                     return null
                }

                // 3. Dup the EGLSync into a SyncFence
                val syncFence: SyncFence = EGLExt.eglDupNativeFenceFDANDROID(eglDisplay, eglSync)

                // 4. Clean up the EGLSync object
                EGL15.eglDestroySync(eglDisplay, eglSync)

                return syncFence
            } finally {
                // Restore previous EGL state
                if (oldDisplay != EGL14.EGL_NO_DISPLAY) {
                    EGL14.eglMakeCurrent(oldDisplay, oldDrawSurface, oldReadSurface, oldContext)
                }
            }
        }
    }

    private const val EGL_SYNC_NATIVE_FENCE_ANDROID = 0x3144
    private const val EGL_SYNC_NATIVE_FENCE_FD_ANDROID = 0x3145
}
