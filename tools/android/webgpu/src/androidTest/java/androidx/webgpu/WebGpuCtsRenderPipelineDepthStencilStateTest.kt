package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsRenderPipelineDepthStencilStateTest {

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

    private fun isDepthOrStencilTextureFormat(format: Int): Boolean {
        // A robust subset of depth/stencil formats
        return when (format) {
            TextureFormat.Depth16Unorm,
            TextureFormat.Depth24Plus,
            TextureFormat.Depth24PlusStencil8,
            TextureFormat.Depth32Float,
            TextureFormat.Depth32FloatStencil8,
            TextureFormat.Stencil8 -> true
            else -> false
        }
    }

    @Test
    fun testFormat() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val formats = intArrayOf(
            TextureFormat.RGBA8Unorm, // Not depth/stencil -> should fail
            TextureFormat.Depth24Plus, // Depth -> should pass
            TextureFormat.Stencil8 // Stencil -> should pass
        )

        for (isAsync in booleanOptions) {
            for (format in formats) {
                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                    device,
                    depthStencil = GPUDepthStencilState(
                        format = format,
                        depthWriteEnabled = 0, // false
                        depthCompare = CompareFunction.Always
                    )
                )

                val success = isDepthOrStencilTextureFormat(format)
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    @Test
    fun testDepthBias() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val topologies = intArrayOf(
            PrimitiveTopology.TriangleList, // Valid for depth bias
            PrimitiveTopology.TriangleStrip, // Valid for depth bias
            PrimitiveTopology.PointList, // Invalid for depth bias
            PrimitiveTopology.LineList  // Invalid for depth bias
        )

        val biasConfigs = arrayOf(
            Triple(0, 0.0f, 0.0f),       // no bias
            Triple(1, 0.0f, 0.0f),       // depth bias
            Triple(0, 1.0f, 0.0f),       // slope scale
            Triple(0, 0.0f, 1.0f)        // clamp
        )

        for (isAsync in booleanOptions) {
            for (topology in topologies) {
                for (config in biasConfigs) {
                    val isTriangleTopology = (topology == PrimitiveTopology.TriangleList || topology == PrimitiveTopology.TriangleStrip)
                    val hasDepthBias = config.first != 0 || config.second != 0.0f || config.third != 0.0f
                    val success = !hasDepthBias || isTriangleTopology

                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                        device,
                        primitive = GPUPrimitiveState(topology = topology),
                        depthStencil = GPUDepthStencilState(
                            format = TextureFormat.Depth24Plus,
                            depthWriteEnabled = 1, // true
                            depthCompare = CompareFunction.LessEqual,
                            depthBias = config.first,
                            depthBiasSlopeScale = config.second,
                            depthBiasClamp = config.third
                        )
                    )

                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
                }
            }
        }
    }
}
