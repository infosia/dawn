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
class WebGpuCtsRenderPipelineOverridesTest {

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
    fun testValidOverrides() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            val code = """
                override a: f32;
                override b: f32 = 2.0;
                
                @vertex fn main() -> @builtin(position) vec4<f32> {
                    return vec4<f32>(a, b, 0.0, 1.0);
                }
            """.trimIndent()

            val descriptor = GPURenderPipelineDescriptor(
                layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                vertex = GPUVertexState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                        )
                    ),
                    entryPoint = "main",
                    buffers = arrayOf(),
                    constants = arrayOf(GPUConstantEntry(key = "a", value = 1.0))
                ),
                fragment = GPUFragmentState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode)
                        )
                    ),
                    entryPoint = "main",
                    targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                ),
                primitive = GPUPrimitiveState(),
                multisample = GPUMultisampleState()
            )

            // Success is true because `a` has no default and is provided. `b` has a default and can be omitted.
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, true, { descriptor })
        }
    }

    @Test
    fun testMissingOverride() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            val code = """
                override a: f32;
                
                @fragment fn main() -> @location(0) vec4<f32> {
                    return vec4<f32>(a, 0.0, 0.0, 1.0);
                }
            """.trimIndent()

            val descriptor = GPURenderPipelineDescriptor(
                layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
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
                            shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                        )
                    ),
                    entryPoint = "main",
                    targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm)),
                    constants = arrayOf() // Intentionally omitted
                ),
                primitive = GPUPrimitiveState(),
                multisample = GPUMultisampleState()
            )

            // Should fail because `a` is missing
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, false, { descriptor })
        }
    }
}
