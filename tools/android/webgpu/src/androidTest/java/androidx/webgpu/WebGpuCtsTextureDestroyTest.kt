package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertThrows
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class WebGpuCtsTextureDestroyTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    private fun getSampledTexture(): GPUTexture {
        val desc = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(16, 16, 1),
            usage = TextureUsage.TextureBinding
        )
        .setFormat(TextureFormat.RGBA8Unorm)
        .build()
        return device.createTexture(desc)
    }

    /**
     * 1. Translated from base
     * Test that it is valid to destroy a texture.
     */
    @Test
    fun testBase(): Unit = runBlocking {
        val texture = getSampledTexture()
        texture.destroy()
    }

    /**
     * 2. Translated from twice
     * Test that it is valid to destroy a destroyed texture.
     */
    @Test
    fun testTwice(): Unit = runBlocking {
        val texture = getSampledTexture()
        texture.destroy()
        texture.destroy()
    }

    /**
     * 3. Translated from invalid_texture
     * Test that invalid textures may be destroyed without generating validation errors.
     */
    @Test
    fun testInvalidTexture(): Unit = runBlocking {
        device.pushErrorScope(ErrorFilter.Validation)

        val invalidDesc = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(device.getLimits().maxTextureDimension2D + 1, 1, 1),
            usage = TextureUsage.TextureBinding
        )
        .setFormat(TextureFormat.RGBA8Unorm)
        .build()

        val invalidTexture = device.createTexture(invalidDesc)

        // Expect error because it's invalid.
        assertThrows(ValidationException::class.java) {
            runBlocking { device.popErrorScope() }
        }

        // This line should not generate an error
        invalidTexture.destroy()
    }

    /**
     * 4. Translated from submit_a_destroyed_texture_as_attachment
     */
    @Test
    fun testSubmitDestroyedTexture_Success(): Unit = runBlocking {
        checkSubmitDestroyedAttachment(
            colorDestroyedBefore = false, colorDestroyedAfter = false,
            depthDestroyedBefore = false, depthDestroyedAfter = false,
            expectSuccess = true
        )
    }

    @Test
    fun testSubmitDestroyedTexture_BeforeEncode(): Unit = runBlocking {
        checkSubmitDestroyedAttachment(
            colorDestroyedBefore = true, colorDestroyedAfter = false,
            depthDestroyedBefore = false, depthDestroyedAfter = false,
            expectSuccess = false
        )
    }

    @Test
    fun testSubmitDestroyedTexture_AfterEncode(): Unit = runBlocking {
        checkSubmitDestroyedAttachment(
            colorDestroyedBefore = false, colorDestroyedAfter = false,
            depthDestroyedBefore = false, depthDestroyedAfter = true,
            expectSuccess = false
        )
    }

    private suspend fun checkSubmitDestroyedAttachment(
        colorDestroyedBefore: Boolean, colorDestroyedAfter: Boolean,
        depthDestroyedBefore: Boolean, depthDestroyedAfter: Boolean,
        expectSuccess: Boolean
    ) {
        val colorTextureDesc = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(16, 16, 1),
            usage = TextureUsage.CopyDst or TextureUsage.RenderAttachment
        )
        .setFormat(TextureFormat.RGBA8Unorm)
        .build()

        val depthTextureDesc = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(16, 16, 1),
            usage = TextureUsage.CopyDst or TextureUsage.RenderAttachment
        )
        .setFormat(TextureFormat.Depth24PlusStencil8)
        .build()

        val colorTexture = device.createTexture(colorTextureDesc)
        val depthTexture = device.createTexture(depthTextureDesc)

        if (colorDestroyedBefore) colorTexture.destroy()
        if (depthDestroyedBefore) depthTexture.destroy()

        device.pushErrorScope(ErrorFilter.Validation)

        val commandEncoder = device.createCommandEncoder()
        
        val colorAttachment = GPURenderPassColorAttachment(
            view = colorTexture.createView(),
            clearValue = GPUColor(r = 0.0, g = 0.0, b = 0.0, a = 0.0),
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store
        )

        val depthAttachment = GPURenderPassDepthStencilAttachment(
            view = depthTexture.createView(),
            depthClearValue = 0.0f,
            depthLoadOp = LoadOp.Clear,
            depthStoreOp = StoreOp.Discard,
            stencilClearValue = 0,
            stencilLoadOp = LoadOp.Clear,
            stencilStoreOp = StoreOp.Discard
        )

        val renderPassDesc = GPURenderPassDescriptor(
            colorAttachments = arrayOf(colorAttachment),
            depthStencilAttachment = depthAttachment
        )

        val renderPass = commandEncoder.beginRenderPass(renderPassDesc)
        renderPass.end()

        val cmd = commandEncoder.finish()

        if (colorDestroyedAfter) colorTexture.destroy()
        if (depthDestroyedAfter) depthTexture.destroy()

        device.queue.submit(arrayOf(cmd))

        if (expectSuccess) {
            device.popErrorScope()
        } else {
             assertThrows(ValidationException::class.java) {
                 runBlocking { device.popErrorScope() }
             }
        }
    }
}
