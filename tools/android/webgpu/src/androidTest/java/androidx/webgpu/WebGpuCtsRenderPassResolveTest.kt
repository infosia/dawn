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
class WebGpuCtsRenderPassResolveTest {

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
    fun testResolveValid() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 1,
                usage = TextureUsage.RenderAttachment
            )
        )

        tryRenderPass(true, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }

    @Test
    fun testResolveSourceNotMultisampled() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 1,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 1,
                usage = TextureUsage.RenderAttachment
            )
        )

        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }

    @Test
    fun testResolveTargetNotSingleSampled() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4, // Invalid for resolve target
                usage = TextureUsage.RenderAttachment
            )
        )

        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }

    @Test
    fun testResolveTargetMissingUsage() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 1,
                usage = TextureUsage.CopySrc // Missing RenderAttachment
            )
        )

        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }

    @Test
    fun testResolveFormatMismatch() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.BGRA8Unorm, // Mismatch
                sampleCount = 1,
                usage = TextureUsage.RenderAttachment
            )
        )

        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }

    @Test
    fun testResolveSizeMismatch() = runBlocking {
        val colorTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.RenderAttachment
            )
        )
        val resolveTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 1), // Mismatch
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 1,
                usage = TextureUsage.RenderAttachment
            )
        )

        tryRenderPass(false, GPURenderPassDescriptor(
            colorAttachments = arrayOf(
                GPURenderPassColorAttachment(
                    view = colorTexture.createView(),
                    resolveTarget = resolveTexture.createView(),
                    clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
                    loadOp = LoadOp.Clear,
                    storeOp = StoreOp.Store
                )
            )
        ))
    }
}
