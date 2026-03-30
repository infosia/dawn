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
class WebGpuCtsLayoutShaderCompatTest {
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

    private fun createPipelineLayout(
        entries: Array<GPUBindGroupLayoutEntry>
    ): GPUPipelineLayout {
        val bgl = device.createBindGroupLayout(GPUBindGroupLayoutDescriptor(entries = entries))
        return device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bgl)))
    }

    @Test
    fun testMismatchBufferType_Compute() = runBlocking {
        // Layout: Uniform
        val entries = arrayOf(
            GPUBindGroupLayoutEntry(
                binding = 0,
                visibility = ShaderStage.Compute,
                buffer = GPUBufferBindingLayout(type = BufferBindingType.Uniform)
            )
        )
        val layout = createPipelineLayout(entries)

        // Shader: Storage (read_write)
        val shaderCode = """
            @group(0) @binding(0) var<storage, read_write> tmp : vec4u;
            @compute @workgroup_size(1)
            fn main() {
              _ = tmp;
            }
        """.trimIndent()

        val shaderModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode)))
        
        var errorThrown = false
        try {
            device.createComputePipeline(
                GPUComputePipelineDescriptor(
                    layout = layout,
                    compute = GPUComputeState(module = shaderModule, entryPoint = "main")
                )
            )
        } catch (e: ValidationException) {
            errorThrown = true
        }

        assert(errorThrown) { "Expected validation error for mismatched buffer types (Uniform vs Storage)" }
    }

    @Test
    fun testMismatchSamplerType_Render() = runBlocking {
        // Layout: Filtering sampler
        val entries = arrayOf(
            GPUBindGroupLayoutEntry(
                binding = 0,
                visibility = ShaderStage.Fragment,
                sampler = GPUSamplerBindingLayout(type = SamplerBindingType.Filtering)
            )
        )
        val layout = createPipelineLayout(entries)

        // Shader: Comparison sampler
        val shaderCode = """
            @group(0) @binding(0) var tmp : sampler_comparison;
            @fragment
            fn main() -> @location(0) vec4f {
              _ = tmp;
              return vec4f();
            }
        """.trimIndent()
        
        val vertexCode = """
            @vertex
            fn main() -> @builtin(position) vec4f {
              return vec4f();
            }
        """.trimIndent()

        val fragmentModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode)))
        val vertexModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(vertexCode)))

        var errorThrown = false
        try {
            device.createRenderPipeline(
                GPURenderPipelineDescriptor(
                    layout = layout,
                    vertex = GPUVertexState(module = vertexModule, entryPoint = "main"),
                    fragment = GPUFragmentState(
                        module = fragmentModule,
                        entryPoint = "main",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    )
                )
            )
        } catch (e: ValidationException) {
            errorThrown = true
        }

        assert(errorThrown) { "Expected validation error for mismatched sampler types (Filtering vs Comparison)" }
    }

    @Test
    fun testPromotionStorageTexture() = runBlocking {
        // Layout: ReadWrite Storage Texture
        val entries = arrayOf(
            GPUBindGroupLayoutEntry(
                binding = 0,
                visibility = ShaderStage.Compute,
                storageTexture = GPUStorageTextureBindingLayout(
                    access = StorageTextureAccess.ReadWrite,
                    format = TextureFormat.R32Float,
                    viewDimension = TextureViewDimension._2D
                )
            )
        )
        val layout = createPipelineLayout(entries)

        // Shader: WriteOnly Storage Texture
        // This is a promotion: pipeline provides ReadWrite, but shader restricts itself to WriteOnly.
        // This should SUCCEED.
        val shaderCode = """
            @group(0) @binding(0) var tmp : texture_storage_2d<r32float, write>;
            @compute @workgroup_size(1)
            fn main() {
              _ = tmp;
            }
        """.trimIndent()

        val shaderModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode)))

        var errorThrown = false
        try {
            device.createComputePipeline(
                GPUComputePipelineDescriptor(
                    layout = layout,
                    compute = GPUComputeState(module = shaderModule, entryPoint = "main")
                )
            )
        } catch (e: ValidationException) {
            errorThrown = true
        }

        assert(!errorThrown) { "Expected success when promoting from ReadWrite layout to WriteOnly shader" }
    }

    @Test
    fun testVisibilityIncomplete_Render() = runBlocking {
        // Layout: Visibility VERTEX only
        val entries = arrayOf(
            GPUBindGroupLayoutEntry(
                binding = 0,
                visibility = ShaderStage.Vertex,
                buffer = GPUBufferBindingLayout(type = BufferBindingType.Uniform)
            )
        )
        val layout = createPipelineLayout(entries)

        // Shader uses it in FRAGMENT
        val shaderCode = """
            @group(0) @binding(0) var<uniform> tmp : vec4u;
            @fragment
            fn main() -> @location(0) vec4f {
              _ = tmp;
              return vec4f();
            }
        """.trimIndent()
        
        val vertexCode = """
            @vertex
            fn main() -> @builtin(position) vec4f {
              return vec4f();
            }
        """.trimIndent()

        val fragmentModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode)))
        val vertexModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(vertexCode)))

        var errorThrown = false
        try {
            device.createRenderPipeline(
                GPURenderPipelineDescriptor(
                    layout = layout,
                    vertex = GPUVertexState(module = vertexModule, entryPoint = "main"),
                    fragment = GPUFragmentState(
                        module = fragmentModule,
                        entryPoint = "main",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    )
                )
            )
        } catch (e: ValidationException) {
            errorThrown = true
        }

        assert(errorThrown) { "Expected validation error when shader uses a binding not marked in visibility" }
    }

    @Test
    fun testExactMatch_Compute() = runBlocking {
        // Layout: Storage
        val entries = arrayOf(
            GPUBindGroupLayoutEntry(
                binding = 0,
                visibility = ShaderStage.Compute,
                buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage)
            )
        )
        val layout = createPipelineLayout(entries)

        // Shader: Storage
        val shaderCode = """
            @group(0) @binding(0) var<storage, read_write> tmp : vec4u;
            @compute @workgroup_size(1)
            fn main() {
              _ = tmp;
            }
        """.trimIndent()

        val shaderModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode)))

        var errorThrown = false
        try {
            device.createComputePipeline(
                GPUComputePipelineDescriptor(
                    layout = layout,
                    compute = GPUComputeState(module = shaderModule, entryPoint = "main")
                )
            )
        } catch (e: ValidationException) {
            errorThrown = true
        }

        assert(!errorThrown) { "Expected success when layout and shader exactly match" }
    }
}
