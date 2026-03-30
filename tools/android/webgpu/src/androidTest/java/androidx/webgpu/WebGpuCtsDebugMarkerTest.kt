package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsDebugMarkerTest {
    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    private fun beginRenderPass(commandEncoder: GPUCommandEncoder): GPURenderPassEncoder {
        val attachmentTexture = device.createTexture(
            GPUTextureDescriptor(
                format = TextureFormat.RGBA8Unorm,
                size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                usage = TextureUsage.RenderAttachment
            )
        )
        return commandEncoder.beginRenderPass(
            GPURenderPassDescriptor(
                colorAttachments = arrayOf(
                    GPURenderPassColorAttachment(
                        view = attachmentTexture.createView(),
                        clearValue = GPUColor(r = 1.0, g = 0.0, b = 0.0, a = 1.0),
                        loadOp = LoadOp.Clear,
                        storeOp = StoreOp.Store
                    )
                )
            )
        )
    }

    @Test
    fun testPushPopCallCountUnbalance_CommandEncoder(): Unit = runBlocking {
        for (pushCount in 1..3) {
            for (popCount in 1..3) {
                device.pushErrorScope(ErrorFilter.Validation)
                var errorThrown = false
                try {
                    val encoder = device.createCommandEncoder()

                    for (i in 0 until pushCount) {
                        encoder.pushDebugGroup("EventStart")
                    }

                    encoder.insertDebugMarker("Marker")

                    for (i in 0 until popCount) {
                        encoder.popDebugGroup()
                    }

                    encoder.finish() // Validation will trigger here

                    try {
                        device.popErrorScope()
                    } catch (e: androidx.webgpu.ValidationException) {
                        errorThrown = true
                    }
                } catch (e: androidx.webgpu.ValidationException) {
                    errorThrown = true
                    try { device.popErrorScope() } catch (ignored: Exception) {}
                }

                val success = pushCount == popCount
                if (success) {
                    assert(!errorThrown) { "Expected success for push=$pushCount, pop=$popCount but got validation error" }
                } else {
                    assert(errorThrown) { "Expected validation error for push=$pushCount, pop=$popCount but got success" }
                }
            }
        }
    }

    @Test
    fun testPushPopCallCountUnbalance_RenderComputePass(): Unit = runBlocking {
        val passTypes = listOf("compute", "render")
        for (passType in passTypes) {
            for (pushCount in 1..3) {
                for (popCount in 1..3) {
                    device.pushErrorScope(ErrorFilter.Validation)
                    var errorThrown = false
                    try {
                        val encoder = device.createCommandEncoder()

                        if (passType == "compute") {
                            val pass = encoder.beginComputePass()
                            for (i in 0 until pushCount) {
                                pass.pushDebugGroup("EventStart")
                            }
                            pass.insertDebugMarker("Marker")
                            for (i in 0 until popCount) {
                                pass.popDebugGroup()
                            }
                            pass.end()
                        } else {
                            val pass = beginRenderPass(encoder)
                            for (i in 0 until pushCount) {
                                pass.pushDebugGroup("EventStart")
                            }
                            pass.insertDebugMarker("Marker")
                            for (i in 0 until popCount) {
                                pass.popDebugGroup()
                            }
                            pass.end()
                        }

                        encoder.finish() // Validation triggers here

                        try {
                            device.popErrorScope()
                        } catch (e: androidx.webgpu.ValidationException) {
                            errorThrown = true
                        }
                    } catch (e: androidx.webgpu.ValidationException) {
                        errorThrown = true
                        try { device.popErrorScope() } catch (ignored: Exception) {}
                    }

                    val success = pushCount == popCount
                    if (success) {
                        assert(!errorThrown) { "Expected success for pass=$passType, push=$pushCount, pop=$popCount but got error" }
                    } else {
                        assert(errorThrown) { "Expected error for pass=$passType, push=$pushCount, pop=$popCount but got success" }
                    }
                }
            }
        }
    }
}
