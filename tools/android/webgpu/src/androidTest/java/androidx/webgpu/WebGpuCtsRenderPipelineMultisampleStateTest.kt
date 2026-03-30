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
class WebGpuCtsRenderPipelineMultisampleStateTest {

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

    @Test
    fun testCount() = runBlocking {
        val counts = intArrayOf(0, 1, 2, 3, 4, 8, 16, 1024)
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (count in counts) {
                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                    device,
                    multisample = GPUMultisampleState(
                        count = count,
                        alphaToCoverageEnabled = false
                    )
                )

                val success = count == 1 || count == 4
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    @Test
    fun testAlphaToCoverageCount() = runBlocking {
        val counts = intArrayOf(1, 4)
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (alphaToCoverageEnabled in booleanOptions) {
                for (count in counts) {
                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                        device,
                        multisample = GPUMultisampleState(
                            count = count,
                            alphaToCoverageEnabled = alphaToCoverageEnabled
                        )
                    )

                    val success = if (alphaToCoverageEnabled) (count == 4) else (count == 1 || count == 4)
                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
                }
            }
        }
    }

    @Test
    fun testAlphaToCoverageSampleMask() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (alphaToCoverageEnabled in booleanOptions) {
                for (hasSampleMaskOutput in booleanOptions) {

                    val shaderCode = if (hasSampleMaskOutput) {
                        """
                        struct Output {
                            @builtin(sample_mask) mask_out: u32,
                            @location(0) color : vec4<f32>,
                        }
                        @fragment fn main() -> Output {
                            var o: Output;
                            o.mask_out = 0xFFFFFFFFu;
                            o.color = vec4<f32>(1.0, 1.0, 1.0, 1.0);
                            return o;
                        }
                        """.trimIndent()
                    } else {
                        WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode
                    }

                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                        device,
                        multisample = GPUMultisampleState(
                            count = 4,
                            alphaToCoverageEnabled = alphaToCoverageEnabled
                        ),
                        fragmentShaderCode = shaderCode
                    )

                    val success = !hasSampleMaskOutput || !alphaToCoverageEnabled
                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
                }
            }
        }
    }
}
