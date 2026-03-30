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
class WebGpuCtsTextureCreateUsageTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    /**
     * 1. Translated from texture_usage 
     * Test creating a texture with illegal usages triggering validation errors.
     */
    @Test
    fun testTextureUsageValidations() {
        runBlocking {
            // [Illegal] RenderAttachment is invalid for 1D dimension
            val descriptor = GPUTextureDescriptor(
                size = GPUExtent3D(4, 1, 1),
                dimension = TextureDimension._1D,
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.RenderAttachment,
                textureBindingViewDimension = GPUTextureBindingViewDimension()
            )

            assertThrows(ValidationException::class.java) {
                device.createTexture(descriptor)
            }
        }
    }

    /**
     * 2. Translated from depthOrArrayLayers_and_mipLevelCount_for_transient_attachments
     * Verifies transient renders must hold strictly single-layered heights.
     */
    @Test
    fun testTransientAttachmentConstraints() {
        runBlocking {
            val usages = TextureUsage.RenderAttachment or TextureUsage.TransientAttachment

            val descriptor = GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 2), // Invalid: depth count must be 1 for transient
                format = TextureFormat.RGBA8Unorm,
                usage = usages,
                textureBindingViewDimension = GPUTextureBindingViewDimension()
            )

            assertThrows(ValidationException::class.java) {
                device.createTexture(descriptor)
            }
        }
    }
}
