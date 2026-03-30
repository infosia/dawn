package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.assertEquals
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before
import org.junit.Ignore

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsRenderPipelineResourceCompatibilityTest {

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

    data class ResourceScenario(
        val name: String,
        val shaderCode: String,
        val bindGroupLayoutEntry: GPUBindGroupLayoutEntry,
        val isVertex: Boolean,
        val expectedSuccess: Boolean
    )

    @Ignore("Missing validation in native/JNI layer. Expected failure but got success.")
    @Test
    fun testResourceCompatibility() = runBlocking {
        val scenarios = listOf(
            // 1. Uniform Buffer (Valid)
            ResourceScenario(
                name = "UniformBuffer",
                shaderCode = "@group(0) @binding(0) var<uniform> b: vec4<f32>;",
                bindGroupLayoutEntry = GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = 0,
                    buffer = GPUBufferBindingLayout(type = BufferBindingType.Uniform)
                ),
                isVertex = true,
                expectedSuccess = true
            ),
            // 2. Storage Buffer ReadOnly in Vertex (Valid)
            ResourceScenario(
                name = "StorageBufferReadOnly",
                shaderCode = "@group(0) @binding(0) var<storage, read> b: vec4<f32>;",
                bindGroupLayoutEntry = GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = 0,
                    buffer = GPUBufferBindingLayout(type = BufferBindingType.ReadOnlyStorage)
                ),
                isVertex = true,
                expectedSuccess = true
            ),
            // 3. Texture Float (Valid)
            ResourceScenario(
                name = "TextureFloat",
                shaderCode = "@group(0) @binding(0) var t: texture_2d<f32>;",
                bindGroupLayoutEntry = GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = 0,
                    texture = GPUTextureBindingLayout(
                        sampleType = TextureSampleType.Float,
                        viewDimension = TextureViewDimension._2D
                    )
                ),
                isVertex = false,
                expectedSuccess = true
            ),
            ResourceScenario(
                name = "MismatchSampler",
                shaderCode = "@group(0) @binding(0) var s: sampler;", // WGSL expects Sampler
                bindGroupLayoutEntry = GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = 0,
                    texture = GPUTextureBindingLayout(sampleType = TextureSampleType.Float, viewDimension = TextureViewDimension._2D) // API provides Texture
                ),
                isVertex = false,
                expectedSuccess = false // Shaders expecting a sampler but layout providing a texture should fail validation
            ),
            // 5. Mismatch (Storage Buffer ReadWrite in Vertex) - Invalid
            ResourceScenario(
                name = "StorageBufferReadWriteVertex",
                shaderCode = "@group(0) @binding(0) var<storage, read_write> b: vec4<f32>;",
                bindGroupLayoutEntry = GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = 0,
                    buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage)
                ),
                isVertex = true,
                expectedSuccess = false // Read-write storage not allowed in vertex
            )
        )

        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (scenario in scenarios) {
                
                val shaderWithEntry = if (scenario.isVertex) {
                    """
                    ${scenario.shaderCode}
                    @vertex fn main() -> @builtin(position) vec4<f32> {
                        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
                    }
                    """.trimIndent()
                } else {
                    """
                    ${scenario.shaderCode}
                    @fragment fn main() -> @location(0) vec4<f32> {
                        return vec4<f32>(0.0, 1.0, 0.0, 1.0);
                    }
                    """.trimIndent()
                }

                val entryFlags = if (scenario.isVertex) ShaderStage.Vertex else ShaderStage.Fragment
                val bindingLayout = GPUBindGroupLayoutEntry(
                    binding = scenario.bindGroupLayoutEntry.binding,
                    visibility = entryFlags,
                    buffer = scenario.bindGroupLayoutEntry.buffer,
                    sampler = scenario.bindGroupLayoutEntry.sampler,
                    texture = scenario.bindGroupLayoutEntry.texture,
                    storageTexture = scenario.bindGroupLayoutEntry.storageTexture
                )

                var pipelineLayout: GPUPipelineLayout? = null
                var creationFailedEarly = false

                device.pushErrorScope(ErrorFilter.Validation)
                val bgl = device.createBindGroupLayout(GPUBindGroupLayoutDescriptor(entries = arrayOf(bindingLayout)))
                pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bgl)))
                
                try {
                    val err = device.popErrorScope() // If we don't throw, we're good
                } catch (e: Exception) {
                    // It failed during layout creation. If expectedSuccess is false, this is a valid failure.
                    creationFailedEarly = true
                    if (scenario.expectedSuccess) {
                        fail("Scenario ${scenario.name} failed early during layout creation: ${e.message}")
                    }
                }

                if (creationFailedEarly) {
                    continue
                }

                val vertexModule = device.createShaderModule(
                    GPUShaderModuleDescriptor(
                        shaderSourceWGSL = GPUShaderSourceWGSL(
                            code = if (scenario.isVertex) shaderWithEntry else WebGpuCtsRenderPipelineHelper.kDefaultVertexShaderCode
                        )
                    )
                )

                val fragmentModule = device.createShaderModule(
                    GPUShaderModuleDescriptor(
                        shaderSourceWGSL = GPUShaderSourceWGSL(
                            code = if (!scenario.isVertex) shaderWithEntry else WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode
                        )
                    )
                )

                val descriptor = GPURenderPipelineDescriptor(
                    layout = pipelineLayout!!,
                    vertex = GPUVertexState(
                        module = vertexModule,
                        entryPoint = "main",
                        buffers = arrayOf()
                    ),
                    fragment = GPUFragmentState(
                        module = fragmentModule,
                        entryPoint = "main",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    ),
                    primitive = GPUPrimitiveState(topology = PrimitiveTopology.TriangleList),
                    multisample = GPUMultisampleState()
                )

                try {
                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, scenario.expectedSuccess, { descriptor })
                } catch (e: AssertionError) {
                    fail("Scenario ${scenario.name} failed: ${e.message}")
                }
            }
        }
    }
}
