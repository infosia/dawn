package androidx.webgpu

import android.graphics.SurfaceTexture
import android.view.Surface
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.Util.windowFromSurface
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assume
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class ObjectTest {
  private lateinit var webGpu: WebGpu
  private lateinit var adapter: GPUAdapter

  @Before
  fun setup() = runBlocking {
    webGpu = createWebGpu()
    adapter = webGpu.instance.requestAdapter()
  }

  @After
  fun teardown() {
    runCatching { webGpu.device.destroy() }
    webGpu.close()
  }

  companion object {
    private const val WIDTH = 600
    private const val HEIGHT = 600
  }

  private fun isRGBA8UnormSupported(gpuSurface: GPUSurface): Boolean {
    val capabilities = gpuSurface.getCapabilities(adapter)
    val desiredFormat = TextureFormat.RGBA8Unorm
    return desiredFormat in capabilities.formats
  }

  @Test
  fun sameObjectCompare() {
    val surfaceTexture = SurfaceTexture(false)
    val androidSurface = Surface(surfaceTexture)

    val gpuSurface = webGpu.instance.createSurface(
      SurfaceDescriptor(
        surfaceSourceAndroidNativeWindow =
          SurfaceSourceAndroidNativeWindow(windowFromSurface(androidSurface))
      )
    )

    if (!isRGBA8UnormSupported(gpuSurface)) {
      // If not, skip this test. It's not a failure, just not applicable.
      Assume.assumeTrue("Adapter does not support TextureFormat.RGBA8Unorm for this surface", false)
    }

    gpuSurface.configure(
      SurfaceConfiguration(
        device = webGpu.device,
        format = TextureFormat.RGBA8Unorm,
        usage = TextureUsage.RenderAttachment,
        width = WIDTH,
        height = HEIGHT
      )
    )

    val texture1 = gpuSurface.getCurrentTexture().texture

    val texture2 = gpuSurface.getCurrentTexture().texture

    assertEquals("== matches two Kotlin objects representing the same Dawn object",texture1, texture2)

    assert(texture1.equals(texture2)) {
      ".equals() matches two Kotlin objects representing the same Dawn object"
    }
  }

  @Test
  fun differentObjectCompare() {
    val surfaceTexture = SurfaceTexture(false)
    val androidSurface = Surface(surfaceTexture)

    val gpuSurface1 = webGpu.instance.createSurface(
      SurfaceDescriptor(
        surfaceSourceAndroidNativeWindow =
          SurfaceSourceAndroidNativeWindow(windowFromSurface(androidSurface))
      )
    )
    val gpuSurface2 = webGpu.instance.createSurface(
      SurfaceDescriptor(
        surfaceSourceAndroidNativeWindow =
          SurfaceSourceAndroidNativeWindow(windowFromSurface(androidSurface))
      )
    )

    assert(!(gpuSurface1 == gpuSurface2)) {
      "== fails to match two Kotlin objects representing different Dawn objects"
    }

    assert(!gpuSurface1.equals(gpuSurface2)) {
      ".equals fails to match two Kotlin objects representing different Dawn objects"
    }
  }
}
