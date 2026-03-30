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
class WebGpuCtsRenderPipelineFloat32BlendableTest {

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

    val kFloat32Formats = intArrayOf(
        TextureFormat.R32Float,
        TextureFormat.RG32Float,
        TextureFormat.RGBA32Float
    )

    @Test
    fun testCreateRenderPipeline() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val enabled = device.getFeatures().features.contains(FeatureName.Float32Blendable)

        for (isAsync in booleanOptions) {
            for (hasBlend in booleanOptions) {
                for (format in kFloat32Formats) {

                    val blendState = if (hasBlend) {
                        GPUBlendState(
                            color = GPUBlendComponent(),
                            alpha = GPUBlendComponent()
                        )
                    } else {
                        null
                    }

                    val targets = arrayOf(
                        GPUColorTargetState(
                            format = format,
                            blend = blendState
                        )
                    )

                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                        device, 
                        targets = targets
                    )

                    val success = enabled || !hasBlend
                    
                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
                }
            }
        }
    }
}
