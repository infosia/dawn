package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsRenderPassDescriptorTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    private fun createTestTexture(
        format: Int = TextureFormat.RGBA8Unorm,
        width: Int = 16,
        height: Int = 16
    ): GPUTexture {
        return device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = width, height = height, depthOrArrayLayers = 1),
                format = format,
                usage = TextureUsage.RenderAttachment or TextureUsage.CopySrc
            )
        )
    }

    private fun getColorAttachment(texture: GPUTexture): GPURenderPassColorAttachment {
        return GPURenderPassColorAttachment(
            view = texture.createView(),
            clearValue = GPUColor(1.0, 0.0, 0.0, 1.0),
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store
        )
    }

    private fun getDepthStencilAttachment(texture: GPUTexture): GPURenderPassDepthStencilAttachment {
        return GPURenderPassDepthStencilAttachment(
            view = texture.createView(),
            depthClearValue = 1f,
            depthLoadOp = LoadOp.Clear,
            depthStoreOp = StoreOp.Store,
            stencilClearValue = 0,
            stencilLoadOp = LoadOp.Clear,
            stencilStoreOp = StoreOp.Store
        )
    }

    private suspend fun tryRenderPass(success: Boolean, descriptor: GPURenderPassDescriptor) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            val commandEncoder = device.createCommandEncoder()
            val renderPass = commandEncoder.beginRenderPass(descriptor)
            renderPass.end()
            commandEncoder.finish()
            device.popErrorScope()
            if (!success) {
                fail("Expected validation error but succeeded")
            }
        } catch (e: Exception) {
            if (success) {
                fail("Expected success but failed with: $e")
            }
        }
    }

    @Test
    fun testOneColorAttachment() = runBlocking {
        val colorTexture = createTestTexture(format = TextureFormat.RGBA8Unorm)
        val descriptor = GPURenderPassDescriptor(
            colorAttachments = arrayOf(getColorAttachment(colorTexture))
        )
        tryRenderPass(true, descriptor)
    }

    @Test
    fun testOneDepthStencilAttachment() = runBlocking {
        val depthStencilTexture = createTestTexture(format = TextureFormat.Depth24PlusStencil8)
        val descriptor = GPURenderPassDescriptor(
            colorAttachments = emptyArray(),
            depthStencilAttachment = getDepthStencilAttachment(depthStencilTexture)
        )
        tryRenderPass(true, descriptor)
    }

    @Test
    fun testEmptyColorAttachments() = runBlocking {
        // Without depth stencil (should fail)
        tryRenderPass(false, GPURenderPassDescriptor(colorAttachments = emptyArray()))

        // With depth stencil (should succeed)
        val depthStencilTexture = createTestTexture(format = TextureFormat.Depth24PlusStencil8)
        tryRenderPass(
            true,
            GPURenderPassDescriptor(
                colorAttachments = emptyArray(),
                depthStencilAttachment = getDepthStencilAttachment(depthStencilTexture)
            )
        )
    }

    @Test
    fun testMaxColorAttachmentsLimit() = runBlocking {
        val maxColorAttachments = device.getLimits().maxColorAttachments

        // Control case: exact max limit should succeed
        val validAttachments = Array(maxColorAttachments) {
            getColorAttachment(createTestTexture(format = TextureFormat.R8Unorm))
        }
        tryRenderPass(true, GPURenderPassDescriptor(colorAttachments = validAttachments))

        // Out of bounds: max + 1 should fail
        val invalidAttachments = Array(maxColorAttachments + 1) {
            getColorAttachment(createTestTexture(format = TextureFormat.R8Unorm))
        }
        tryRenderPass(false, GPURenderPassDescriptor(colorAttachments = invalidAttachments))
    }

    @Test
    fun testAttachmentsSameSize() = runBlocking {
        val colorTexture1x1A = createTestTexture(width = 1, height = 1)
        val colorTexture1x1B = createTestTexture(width = 1, height = 1)
        val colorTexture2x2 = createTestTexture(width = 2, height = 2)
        val depthStencilTexture1x1 = createTestTexture(width = 1, height = 1, format = TextureFormat.Depth24PlusStencil8)
        val depthStencilTexture2x2 = createTestTexture(width = 2, height = 2, format = TextureFormat.Depth24PlusStencil8)

        // Control case: all the same size (1x1) - should succeed
        tryRenderPass(true, GPURenderPassDescriptor(
            colorAttachments = arrayOf(getColorAttachment(colorTexture1x1A), getColorAttachment(colorTexture1x1B)),
            depthStencilAttachment = getDepthStencilAttachment(depthStencilTexture1x1)
        ))

        // One color attachment has a different size - should fail
        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(getColorAttachment(colorTexture1x1A), getColorAttachment(colorTexture2x2))
        ))

        // Depth stencil attachment has a different size - should fail
        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(getColorAttachment(colorTexture1x1A), getColorAttachment(colorTexture1x1B)),
            depthStencilAttachment = getDepthStencilAttachment(depthStencilTexture2x2)
        ))
    }

    @Test
    fun testAttachmentsColorDepthMismatch() = runBlocking {
        val colorTexture = createTestTexture(format = TextureFormat.RGBA8Unorm)
        val depthStencilTexture = createTestTexture(format = TextureFormat.Depth24PlusStencil8)

        // Using depth-stencil for color
        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(getColorAttachment(depthStencilTexture))
        ))

        // Using color for depth-stencil
        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = emptyArray(),
            depthStencilAttachment = getDepthStencilAttachment(colorTexture)
        ))
    }
}
