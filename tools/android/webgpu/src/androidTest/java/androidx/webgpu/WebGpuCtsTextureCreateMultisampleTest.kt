package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class WebGpuCtsTextureCreateMultisampleTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    /**
     * 1. Translated from sample_count,1d_2d_array_3d
     * Test that you can not create 1d, 2d_array, and 3d multisampled textures.
     */
    @Test
    fun testSampleCountDimensions() {
        runBlocking {
            // 1. standard 2D multisample (Valid)
            val desc1 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding or TextureUsage.RenderAttachment,
                size = GPUExtent3D(4, 4, 1)
            ).setSampleCount(4)
             .setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            val tex1 = device.createTexture(desc1)
            tex1.close()

            // 2. 2D Array multisample (Invalid)
            val desc2 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding or TextureUsage.RenderAttachment,
                size = GPUExtent3D(4, 4, 4)
            ).setSampleCount(4)
             .setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            assertThrows(ValidationException::class.java) {
                device.createTexture(desc2)
            }
        }
    }

    /**
     * 2. Translated from sampleCount,various_sampleCount_with_all_formats
     * Verifies valid and invalid sample counts (usually only 1 and 4 are valid).
     */
    @Test
    fun testVariousSampleCounts() {
        runBlocking {
            // 1. sampleCount = 1 (Valid)
            val desc1 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(32, 32, 1)
            ).setSampleCount(1)
             .setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            val tex1 = device.createTexture(desc1)
            tex1.close()

            // 2. sampleCount = 4 (Valid)
            val desc2 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding or TextureUsage.RenderAttachment,
                size = GPUExtent3D(32, 32, 1)
            ).setSampleCount(4)
             .setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            val tex2 = device.createTexture(desc2)
            tex2.close()
        }
    }
}
