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
class WebGpuCtsRenderPipelinePrimitiveStateTest {

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

    val kPrimitiveTopology = intArrayOf(
        PrimitiveTopology.PointList,
        PrimitiveTopology.LineList,
        PrimitiveTopology.LineStrip,
        PrimitiveTopology.TriangleList,
        PrimitiveTopology.TriangleStrip,
        -1 // Undefined
    )

    val kIndexFormat = intArrayOf(
        IndexFormat.Uint16,
        IndexFormat.Uint32,
        -1 // Undefined
    )

    @Test
    fun testStripIndexFormat() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (topology in kPrimitiveTopology) {
                for (stripIndexFormat in kIndexFormat) {
                    val primitive = if (topology == -1 && stripIndexFormat == -1) {
                        GPUPrimitiveState()
                    } else if (topology == -1) {
                        GPUPrimitiveState(stripIndexFormat = stripIndexFormat)
                    } else if (stripIndexFormat == -1) {
                        GPUPrimitiveState(topology = topology)
                    } else {
                        GPUPrimitiveState(topology = topology, stripIndexFormat = stripIndexFormat)
                    }
                    
                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(device, primitive = primitive)

                    val success = (topology == PrimitiveTopology.LineStrip) || 
                                  (topology == PrimitiveTopology.TriangleStrip) || 
                                  (stripIndexFormat == -1)
                    
                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
                }
            }
        }
    }

    @Test
    fun testUnclippedDepth() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (unclippedDepth in booleanOptions) {
                val primitive = GPUPrimitiveState(unclippedDepth = unclippedDepth)
                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(device, primitive = primitive)

                val success = !unclippedDepth || device.getFeatures().features.contains(FeatureName.DepthClipControl)
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }
}
