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
class WebGpuCtsRenderPipelineFragmentStateTest {

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
    fun testTargetFormat() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val formats = intArrayOf(
            TextureFormat.RGBA8Unorm, // Valid render target
            TextureFormat.BGRA8Unorm, // Valid render target
            TextureFormat.RGBA32Float, // Valid render target
            TextureFormat.Depth24Plus // Invalid color target format
        )

        for (isAsync in booleanOptions) {
            for (format in formats) {

                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                    device,
                    targets = arrayOf(GPUColorTargetState(format = format))
                )

                val success = format != TextureFormat.Depth24Plus
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    @Test
    fun testColorWriteMask() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val masks = intArrayOf(
            ColorWriteMask.All,
            ColorWriteMask.Red or ColorWriteMask.Alpha,
            ColorWriteMask.None, // Might be 0 but check if it's supported
            0 // None
        )

        for (isAsync in booleanOptions) {
            for (mask in masks) {

                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                    device,
                    targets = arrayOf(GPUColorTargetState(
                        format = TextureFormat.RGBA8Unorm,
                        writeMask = mask
                    ))
                )

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, true, { descriptor })
            }
        }
    }

    @Test
    fun testMaxColorTargets() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val maxTargets = device.getLimits().maxColorAttachments

        // Test with maxTargets, maxTargets+1
        val limitsToTest = intArrayOf(maxTargets, maxTargets + 1)

        for (isAsync in booleanOptions) {
            for (limit in limitsToTest) {
                
                val targets = mutableListOf<GPUColorTargetState>()
                var fragmentCode = ""
                for (i in 0 until limit) {
                    targets.add(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    fragmentCode += "@location($i) color$i: vec4<f32>,\n"
                }

                val fullFragmentCode = """
                    struct FragmentOut {
                        $fragmentCode
                    }
                    @fragment fn main() -> FragmentOut {
                        var out: FragmentOut;
                        return out;
                    }
                """.trimIndent()

                val bgl = device.createBindGroupLayout(GPUBindGroupLayoutDescriptor(entries = arrayOf()))
                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bgl)))

                val descriptor = GPURenderPipelineDescriptor(
                    layout = pipelineLayout,
                    vertex = GPUVertexState(
                        module = device.createShaderModule(
                            GPUShaderModuleDescriptor(
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultVertexShaderCode)
                            )
                        ),
                        entryPoint = "main",
                        buffers = arrayOf()
                    ),
                    fragment = GPUFragmentState(
                        module = device.createShaderModule(
                            GPUShaderModuleDescriptor(
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = fullFragmentCode)
                            )
                        ),
                        entryPoint = "main",
                        targets = targets.toTypedArray()
                    ),
                    primitive = GPUPrimitiveState(),
                    multisample = GPUMultisampleState()
                )

                val maxBytes = device.getLimits().maxColorAttachmentBytesPerSample
                // RGBA8Unorm takes 4 bytes, but on some backends might be aligned to more.
                // We'll just pass success = false if limit > maxTargets OR (limit * 4) > maxBytes.
                val success = (limit <= maxTargets) && (limit * 8 <= maxBytes) // Assuming 8-byte alignment per target in Dawn/WGPU 
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }
}
