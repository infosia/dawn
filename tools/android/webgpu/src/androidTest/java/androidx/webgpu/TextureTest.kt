package androidx.webgpu

import androidx.test.filters.SmallTest
import androidx.webgpu.helper.UncapturedErrorException
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import junit.framework.TestCase
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Test

@SmallTest
class TextureTest {
  private lateinit var webGpu: WebGpu
  private lateinit var device: Device

  @Before
  fun setup() = runBlocking {
    webGpu = createWebGpu()
    device = webGpu.device
  }

  @After
  fun teardown() {
    runCatching { device.destroy() }
    webGpu.close()
  }


  @Test
  fun testTextureProperties() {
    val testTextureDescriptor = TextureDescriptor(
      size = Extent3D(width = 32, height = 16, depthOrArrayLayers = 1),
      format = TextureFormat.RGBA8Unorm,
      usage = TextureUsage.TextureBinding or TextureUsage.CopyDst,
      mipLevelCount = 1,
      sampleCount = 1,
      dimension = TextureDimension._2D
    )
    val texture = device.createTexture(testTextureDescriptor)
    TestCase.assertEquals(texture.width, testTextureDescriptor.size.width)
    TestCase.assertEquals(texture.height, testTextureDescriptor.size.height)
    TestCase.assertEquals(texture.depthOrArrayLayers, testTextureDescriptor.size.depthOrArrayLayers)
    TestCase.assertEquals(texture.format, testTextureDescriptor.format)
    TestCase.assertEquals(texture.usage, testTextureDescriptor.usage)
    TestCase.assertEquals(texture.mipLevelCount, testTextureDescriptor.mipLevelCount)
    TestCase.assertEquals(texture.sampleCount, testTextureDescriptor.sampleCount)
    TestCase.assertEquals(texture.dimension, testTextureDescriptor.dimension)
    texture.destroy()
  }

  /**
   * Verifies that using a view from a destroyed texture generates an uncaptured error
   * upon submission to the queue.
   */
  @Test
  fun useOfDestroyedTextureView_firesUncapturedError() {
    val textureDescriptor = TextureDescriptor(
      size = Extent3D(width = 32, height = 16, depthOrArrayLayers = 1),
      format = TextureFormat.RGBA8Unorm,
      usage = TextureUsage.RenderAttachment
    )

    val texture = device.createTexture(textureDescriptor)
    texture.destroy()

    val invalidView = texture.createView()

    val encoder = device.createCommandEncoder()
    val passEncoder = encoder.beginRenderPass(
      RenderPassDescriptor(
        colorAttachments = arrayOf(
          RenderPassColorAttachment(
            view = invalidView,
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store,
            clearValue = Color(r = 1.0, g = 0.0, b = 0.0, a = 1.0)
          )
        )
      )
    )
    passEncoder.end()
    val commandBuffer = encoder.finish()

    val queue = device.getQueue()
    assertThrows(UncapturedErrorException::class.java) {
      queue.submit(arrayOf(commandBuffer))
      runBlocking {
        queue.onSubmittedWorkDone()
      }
    }
  }
}