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
class WebGpuCtsNonFilterableTextureTest {
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

    private fun testFilteringSamplerWithTexture(
        sampleType: Int,
        wgslTextureType: String,
        wgslComponent: String,
        isCompute: Boolean
    ) = runBlocking {
        // Layout: Texture (of sampleType) + Sampler (Filtering)
        val bindGroupLayout = device.createBindGroupLayout(
            GPUBindGroupLayoutDescriptor(
                entries = arrayOf(
                    GPUBindGroupLayoutEntry.Builder(
                        binding = 0,
                        visibility = ShaderStage.Compute or ShaderStage.Fragment
                    ).setTexture(GPUTextureBindingLayout(sampleType = sampleType, viewDimension = TextureViewDimension._2D)).build(),
                    GPUBindGroupLayoutEntry.Builder(
                        binding = 1,
                        visibility = ShaderStage.Compute or ShaderStage.Fragment
                    ).setSampler(GPUSamplerBindingLayout(type = SamplerBindingType.Filtering)).build()
                )
            )
        )

        val pipelineLayout = device.createPipelineLayout(
            GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bindGroupLayout))
        )

        val successExpected = (sampleType == TextureSampleType.Float)

        val shaderCode = """
            @group(0) @binding(0) var t: $wgslTextureType;
            @group(0) @binding(1) var s: sampler;

            fn evaluate() {
                _ = textureGather($wgslComponent t, s, vec2f(0.0, 0.0));
            }

            @compute @workgroup_size(1) 
            fn cs() {
                evaluate();
            }

            @vertex 
            fn vs() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 1.0);
            }

            @fragment 
            fn fs() -> @location(0) vec4f {
                evaluate();
                return vec4f(0.0, 0.0, 0.0, 0.0);
            }
        """.trimIndent()

        val vertexShaderModule = device.createShaderModule(
            GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode))
        )
        val fragmentShaderModule = device.createShaderModule(
            GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(shaderCode))
        )

        var errorThrown = false
        try {
            android.util.Log.i("WebGpuTest", "Trying create compute for Uint...")
            if (isCompute) {
                device.createComputePipeline(
                    GPUComputePipelineDescriptor(
                        layout = pipelineLayout,
                        compute = GPUComputeState(module = vertexShaderModule, entryPoint = "cs")
                    )
                )
            } else {
                android.util.Log.i("WebGpuTest", "Trying create render for Uint...")
                device.createRenderPipeline(
                    GPURenderPipelineDescriptor(
                        layout = pipelineLayout,
                        vertex = GPUVertexState(module = vertexShaderModule, entryPoint = "vs"),
                        fragment = GPUFragmentState(
                            module = fragmentShaderModule,
                            entryPoint = "fs",
                            targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                        )
                    )
                )
            }
        } catch (e: ValidationException) {
            errorThrown = true
        }

        if (successExpected) {
            assert(!errorThrown) { "Expected success when linking Float texture with filtering sampler" }
        } else {
            assert(errorThrown) { "Expected validation error when linking unfilterable texture with filtering sampler" }
        }
    }

    @Test
    fun testFloatTexture_Valid() {
        testFilteringSamplerWithTexture(TextureSampleType.Float, "texture_2d<f32>", "0,", isCompute = true)
        testFilteringSamplerWithTexture(TextureSampleType.Float, "texture_2d<f32>", "0,", isCompute = false)
    }

    @Test
    fun testSintTexture_Invalid() {
        testFilteringSamplerWithTexture(TextureSampleType.Sint, "texture_2d<i32>", "0,", isCompute = true)
        // BUG: Dawn Native throws SIGBUS on createRenderPipeline validation for this combination
        // testFilteringSamplerWithTexture(TextureSampleType.Sint, "texture_2d<i32>", "0,", isCompute = false)
    }

    @Test
    fun testUintTexture_Invalid() {
        testFilteringSamplerWithTexture(TextureSampleType.Uint, "texture_2d<u32>", "0,", isCompute = true)
        // BUG: Dawn Native throws SIGBUS on createRenderPipeline validation for this combination
        // testFilteringSamplerWithTexture(TextureSampleType.Uint, "texture_2d<u32>", "0,", isCompute = false)
    }

    @Test
    fun testDepthTexture_Invalid() {
        testFilteringSamplerWithTexture(TextureSampleType.Depth, "texture_depth_2d", "", isCompute = true)
        // BUG: Dawn Native throws SIGBUS on createRenderPipeline validation for this combination
        // testFilteringSamplerWithTexture(TextureSampleType.Depth, "texture_depth_2d", "", isCompute = false)
    }
}
