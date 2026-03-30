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
class WebGpuCtsRenderPassAttachmentCompatibilityTest {

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

    private fun createAttachmentTextureView(format: Int, sampleCount: Int = 1): GPUTextureView {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                format = format,
                usage = TextureUsage.RenderAttachment or TextureUsage.CopySrc,
                sampleCount = sampleCount
            )
        )
        return texture.createView()
    }

    private fun createColorAttachment(format: Int?, sampleCount: Int = 1): GPURenderPassColorAttachment? {
        return if (format == null) null else GPURenderPassColorAttachment(
            view = createAttachmentTextureView(format, sampleCount),
            clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store
        )
    }

    @Test
    fun testRenderPassAndBundleColorFormat() = runBlocking {
        val formats = arrayOf(TextureFormat.RGBA8Unorm, TextureFormat.BGRA8Unorm, TextureFormat.RGBA8Uint)
        
        for (passFormat in formats) {
            for (bundleFormat in formats) {
                var expectedSuccess = (passFormat == bundleFormat)
                
                device.pushErrorScope(ErrorFilter.Validation)
                try {
                    val bundleEncoder = device.createRenderBundleEncoder(
                        GPURenderBundleEncoderDescriptor(colorFormats = intArrayOf(bundleFormat))
                    )
                    val bundle = bundleEncoder.finish()

                    val encoder = device.createCommandEncoder()
                    val pass = encoder.beginRenderPass(
                        GPURenderPassDescriptor(
                            colorAttachments = arrayOf(createColorAttachment(passFormat)!!)
                        )
                    )
                    pass.executeBundles(arrayOf(bundle))
                    pass.end()
                    encoder.finish()
                    device.popErrorScope()
                    
                    if (!expectedSuccess) {
                        fail("Expected failure but succeeded with mismatch formats pass=$passFormat bundle=$bundleFormat")
                    }
                } catch (e: Exception) {
                    if (expectedSuccess) {
                        fail("Expected success but failed with $e")
                    }
                }
            }
        }
    }

    @Test
    fun testRenderPassAndBundleColorCount() = runBlocking {
        val counts = arrayOf(1, 2, 3, 4)
        val limit = device.getLimits().maxColorAttachments
        
        for (passCount in counts) {
            if (passCount > limit) continue
            for (bundleCount in counts) {
                if (bundleCount > limit) continue
                
                var expectedSuccess = (passCount == bundleCount)
                
                device.pushErrorScope(ErrorFilter.Validation)
                try {
                    val bundleFormats = Array(bundleCount) { TextureFormat.RGBA8Uint }
                    val bundleEncoder = device.createRenderBundleEncoder(
                        GPURenderBundleEncoderDescriptor(colorFormats = bundleFormats.toIntArray())
                    )
                    val bundle = bundleEncoder.finish()

                    val encoder = device.createCommandEncoder()
                    val colorAttachments = Array(passCount) { createColorAttachment(TextureFormat.RGBA8Uint)!! }
                    val pass = encoder.beginRenderPass(
                        GPURenderPassDescriptor(colorAttachments = colorAttachments)
                    )

                    pass.executeBundles(arrayOf(bundle))
                    pass.end()
                    encoder.finish()
                    device.popErrorScope()
                    
                    if (!expectedSuccess) {
                        fail("Expected failure but succeeded with mismatch limits pass=$passCount bundle=$bundleCount")
                    }
                } catch (e: Exception) {
                    if (expectedSuccess) {
                        fail("Expected success but failed with $e")
                    }
                }
            }
        }
    }
}
