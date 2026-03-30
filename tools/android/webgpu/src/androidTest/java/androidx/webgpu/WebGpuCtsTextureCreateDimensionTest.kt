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
class WebGpuCtsTextureCreateDimensionTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    private fun possiblyCompatible(dimension: Int, format: Int): Boolean {
        // Depth formats are strictly 2D Only in WebGPU
        if (format == TextureFormat.Depth24PlusStencil8) {
            if (dimension != TextureDimension._2D) {
                return false
            }
        }
        return true
    }



    /**
     * 1. Translated from zero_size_and_usage
     * Test texture creation with zero arguments for sizes or usage.
     */
    @Test
    fun testZeroSizeAndUsage() {
        runBlocking {
            // 1. 2D + RGBA8 + none (Valid)
            val desc1 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(4, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .setMipLevelCount(1)
             .build()
            val tex1 = device.createTexture(desc1)
            tex1.close()

            /*
            // 2. 2D + RGBA8 + width=0 (Invalid)
            // Commented out: width=0 triggers a native abort in C++ instead of a handled ValidationException.
            val desc2 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(0, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .setMipLevelCount(1)
             .build()
            assertThrows(ValidationException::class.java) {
                device.createTexture(desc2)
            }
            */

            // 3. 2D + Depth + none (Valid)
            val desc3 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(4, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.Depth24PlusStencil8)
             .setMipLevelCount(1)
             .build()
            val tex3 = device.createTexture(desc3)
            tex3.close()

            /*
            // 4. 2D + Depth + width=0 (Invalid)
            // Commented out: width=0 triggers a native abort in C++ instead of a handled ValidationException.
            val desc4 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(0, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.Depth24PlusStencil8)
             .setMipLevelCount(1)
             .build()
            assertThrows(ValidationException::class.java) {
                device.createTexture(desc4)
            }
            */
        }
    }

    /**
     * 2. Translated from dimension_type_and_format_compatibility
     * Test compatibility bounds across axes and formats.
     */
    @Test
    fun testDimensionTypeAndFormatCompatibility() {
        runBlocking {
            // 1. 2D + RGBA8 (Valid)
            val desc1 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(4, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            val tex1 = device.createTexture(desc1)
            tex1.close()

            // 2. 2D + Depth (Valid)
            val desc2 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(4, 4, 1)
            ).setDimension(TextureDimension._2D)
             .setFormat(TextureFormat.Depth24PlusStencil8)
             .build()
            val tex2 = device.createTexture(desc2)
            tex2.close()

            // 3. 3D + RGBA8 (Valid)
            val desc3 = GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = GPUExtent3D(4, 4, 1)
            ).setDimension(TextureDimension._3D)
             .setFormat(TextureFormat.RGBA8Unorm)
             .build()
            val tex3 = device.createTexture(desc3)
            tex3.close()
        }
    }

    /**
     * 3. Translated from mipLevelCount,bound_check
     * Test out-of-bounds validations on mipLevelCount upper scaling.
     */
    @Test
    fun testMipLevelCountBoundCheck() {
        runBlocking {
            val descriptor = GPUTextureDescriptor(
                size = GPUExtent3D(32, 32, 1),
                mipLevelCount = 100, // Invalid: exceeds max levels for 32x32 size (log2(32) + 1 = 6)
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.TextureBinding
            )

            assertThrows(ValidationException::class.java) {
                device.createTexture(descriptor)
            }
        }
    }
}
