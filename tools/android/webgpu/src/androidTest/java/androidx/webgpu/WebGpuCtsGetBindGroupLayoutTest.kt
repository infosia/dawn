package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import org.junit.Ignore
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class WebGpuCtsGetBindGroupLayoutTest {

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

    @Ignore("Missing validation in native/JNI layer. Expected failure but got success.")
    @Test
    fun testIndexRangeExplicitLayout() = runBlocking {
        val pipelineBindGroupLayouts = device.createBindGroupLayout()

        val pipelineLayout = device.createPipelineLayout(
            GPUPipelineLayoutDescriptor(
                bindGroupLayouts = arrayOf(pipelineBindGroupLayouts)
            )
        )

        val pipeline = device.createRenderPipeline(
            GPURenderPipelineDescriptor(
                layout = pipelineLayout,
                vertex = GPUVertexState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL("""
                                @vertex
                                fn main()-> @builtin(position) vec4<f32> {
                                  return vec4<f32>(0.0, 0.0, 0.0, 1.0);
                                }
                            """.trimIndent())
                        )
                    ),
                    entryPoint = "main"
                ),
                fragment = GPUFragmentState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL("""
                                @fragment
                                fn main() -> @location(0) vec4<f32> {
                                  return vec4<f32>(0.0, 1.0, 0.0, 1.0);
                                }
                            """.trimIndent())
                        )
                    ),
                    entryPoint = "main",
                    targets = arrayOf(
                        GPUColorTargetState(
                            format = TextureFormat.RGBA8Unorm,
                            blend = null,
                            writeMask = ColorWriteMask.All
                        )
                    )
                ),
                primitive = GPUPrimitiveState(),
                depthStencil = null,
                multisample = GPUMultisampleState()
            )
        )

        val maxBindGroups = device.getLimits().maxBindGroups
        
        for (index in 0..maxBindGroups + 2) {
            val shouldError = index >= 1
            var errorThrown = false
            try {
                device.pushErrorScope(ErrorFilter.Validation)
                pipeline.getBindGroupLayout(index)
                val error = device.popErrorScope()
                if (error != ErrorType.NoError) {
                    errorThrown = true
                }
            } catch (e: Exception) {
                errorThrown = true
            }

            if (shouldError) {
                assert(errorThrown) { "Expected validation error for index $index, but got success" }
            } else {
                assert(!errorThrown) { "Expected success for index $index, but got validation error" }
            }
        }
    }

    @Ignore("Missing validation in native/JNI layer. Expected failure but got success.")
    @Test
    fun testIndexRangeAutoLayout() = runBlocking {
        val pipeline = device.createRenderPipeline(
            GPURenderPipelineDescriptor(
                layout = null, // 'auto'
                vertex = GPUVertexState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL("""
                                @vertex
                                fn main()-> @builtin(position) vec4<f32> {
                                  return vec4<f32>(0.0, 0.0, 0.0, 1.0);
                                }
                            """.trimIndent())
                        )
                    ),
                    entryPoint = "main"
                ),
                fragment = GPUFragmentState(
                    module = device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL("""
                                @group(0) @binding(0) var<uniform> binding: f32;
                                @fragment
                                fn main() -> @location(0) vec4<f32> {
                                  _ = binding;
                                  return vec4<f32>(0.0, 1.0, 0.0, 1.0);
                                }
                            """.trimIndent())
                        )
                    ),
                    entryPoint = "main",
                    targets = arrayOf(
                        GPUColorTargetState(
                            format = TextureFormat.RGBA8Unorm,
                            blend = null,
                            writeMask = ColorWriteMask.All
                        )
                    )
                ),
                primitive = GPUPrimitiveState(),
                depthStencil = null,
                multisample = GPUMultisampleState()
            )
        )

        val maxBindGroups = device.getLimits().maxBindGroups
        
        for (index in 0..maxBindGroups + 2) {
            val shouldError = index >= 1
            var errorThrown = false
            try {
                device.pushErrorScope(ErrorFilter.Validation)
                pipeline.getBindGroupLayout(index)
                val error = device.popErrorScope()
                if (error != ErrorType.NoError) {
                    errorThrown = true
                }
            } catch (e: Exception) {
                errorThrown = true
            }

            if (shouldError) {
                assert(errorThrown) { "Expected validation error for index $index, but got success" }
            } else {
                assert(!errorThrown) { "Expected success for index $index, but got validation error" }
            }
        }
    }
}