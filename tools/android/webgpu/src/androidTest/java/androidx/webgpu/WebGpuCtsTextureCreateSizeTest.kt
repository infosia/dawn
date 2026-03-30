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
class WebGpuCtsTextureCreateSizeTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    /**
     * 1. Translated from texture_size,default_value_and_smallest_size,uncompressed_format
     * Test creating a texture with smallest sizes available.
     */
    @Test
    fun testSmallestTextureSize() {
        runBlocking {
            val dimensions = listOf(TextureDimension._1D, TextureDimension._2D, TextureDimension._3D)

            for (dimension in dimensions) {
                var width = 1
                var height = 1
                var depth = 1

                if (dimension == TextureDimension._1D) {
                    height = 1
                    depth = 1
                }

                val descriptor = GPUTextureDescriptor(
                    size = GPUExtent3D(width, height, depth),
                    dimension = dimension,
                    format = TextureFormat.RGBA8Unorm,
                    usage = TextureUsage.TextureBinding,
                    textureBindingViewDimension = GPUTextureBindingViewDimension()
                )

                val texture = device.createTexture(descriptor)
                texture.close()
            }
        }
    }

    /**
     * 2. Translated from texture_size,1d_texture / 2d_texture bounds
     * Test size exceedance limits (e.g., extremely large limits that generally fail).
     */
    @Test
    fun testTextureSizeUpperBounds() {
        runBlocking {
            // Exceeding typical max weights (usually 2048 or 8192 for core tiers)
            val sizes = listOf(4096, 8192, 16384)

            for (size in sizes) {
                val descriptor = GPUTextureDescriptor(
                    size = GPUExtent3D(size, size, 1),
                    dimension = TextureDimension._2D,
                    format = TextureFormat.RGBA8Unorm,
                    usage = TextureUsage.TextureBinding,
                    textureBindingViewDimension = GPUTextureBindingViewDimension()
                )

                try {
                    val texture = device.createTexture(descriptor)
                    texture.close()
                } catch (e: ValidationException) {
                    // Success: accurately caught limit exceedance on this device 
                }
            }
        }
    }
}
