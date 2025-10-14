package androidx.webgpu

import android.graphics.SurfaceTexture
import android.view.Surface
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import junit.framework.TestCase
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class SurfaceTest {

  @Test
  fun surfaceTest() {
    val width = 600
    val height = 800
    val surfaceTexture = SurfaceTexture(false)
    surfaceTexture.setDefaultBufferSize(width, height)
    val validSurface = Surface(surfaceTexture)
    var webGpu: WebGpu? = null
    try {
      webGpu = runBlocking { createWebGpu(validSurface) }
      val surface = webGpu.webgpuSurface
      val config = SurfaceConfiguration(
        device = webGpu.device,
        format = TextureFormat.RGBA8Unorm,
        height = height,
        width = width
      )
      surface.configure(config)
      // ensure no errors are thrown
      surface.getCurrentTexture()
      val status = surface.present()
      TestCase.assertEquals("Present should be successful", Status.Success, status)
    } finally {
      validSurface.release()
      surfaceTexture.release()
      webGpu?.close()
    }
  }
}