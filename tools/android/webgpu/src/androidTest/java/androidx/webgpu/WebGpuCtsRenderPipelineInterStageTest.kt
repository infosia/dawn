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
class WebGpuCtsRenderPipelineInterStageTest {

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

    private fun getVertexStateWithOutputs(outputs: Array<String>): GPUVertexState {
        val mappedOutputs = outputs.mapIndexed { i, v -> v.replace("__", "v$i") }.joinToString(",\n            ")
        val code = """
            struct A {
                $mappedOutputs${if(outputs.isNotEmpty()) "," else ""}
                @builtin(position) pos: vec4<f32>,
            }
            @vertex fn main() -> A {
                var vertexOut: A;
                vertexOut.pos = vec4<f32>(0.0, 0.0, 0.0, 1.0);
                return vertexOut;
            }
        """.trimIndent()

        return GPUVertexState(
            module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            ),
            entryPoint = "main",
            buffers = arrayOf()
        )
    }

    private fun getFragmentStateWithInputs(inputs: Array<String>): GPUFragmentState {
        val mappedInputs = inputs.mapIndexed { i, v -> v.replace("__", "v$i") }.joinToString(",\n            ")
        val code = """
            struct B {
                $mappedInputs
            }
            @fragment fn main(fragmentIn: B) -> @location(0) vec4<f32> {
                return vec4<f32>(1.0, 1.0, 1.0, 1.0);
            }
        """.trimIndent()

        return GPUFragmentState(
            targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm)),
            module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            ),
            entryPoint = "main"
        )
    }

    @Test
    fun testLocationMismatch() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val scenarios = listOf(
            Triple(arrayOf("@location(0) __: f32"), arrayOf("@location(0) __: f32"), true),
            Triple(arrayOf("@location(0) __: f32"), arrayOf("@location(1) __: f32"), false),
            Triple(arrayOf("@location(1) __: f32"), arrayOf("@location(0) __: f32"), false),
            Triple(arrayOf("@location(0) __: f32", "@location(1) __: f32"), arrayOf("@location(1) __: f32", "@location(0) __: f32"), true),
            Triple(arrayOf("@location(1) __: f32", "@location(0) __: f32"), arrayOf("@location(0) __: f32", "@location(1) __: f32"), true)
        )

        for (isAsync in booleanOptions) {
            for (scenario in scenarios) {
                val descriptor = GPURenderPipelineDescriptor(
                    layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                    vertex = getVertexStateWithOutputs(scenario.first),
                    fragment = getFragmentStateWithInputs(scenario.second),
                    primitive = GPUPrimitiveState(topology = PrimitiveTopology.TriangleList),
                    multisample = GPUMultisampleState()
                )

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, scenario.third, { descriptor })
            }
        }
    }

    @Test
    fun testLocationSupersetAndSubset() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            // Superset (Success)
            var descriptor = GPURenderPipelineDescriptor(
                layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                vertex = getVertexStateWithOutputs(arrayOf("@location(0) vout0: f32", "@location(1) vout1: f32")),
                fragment = getFragmentStateWithInputs(arrayOf("@location(1) fin1: f32")),
                primitive = GPUPrimitiveState(),
                multisample = GPUMultisampleState()
            )
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, true, { descriptor })

            // Subset (Fail)
            descriptor = GPURenderPipelineDescriptor(
                layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                vertex = getVertexStateWithOutputs(arrayOf("@location(0) vout0: f32")),
                fragment = getFragmentStateWithInputs(arrayOf("@location(0) fin0: f32", "@location(1) fin1: f32")),
                primitive = GPUPrimitiveState(),
                multisample = GPUMultisampleState()
            )
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, false, { descriptor })
        }
    }

    @Test
    fun testTypeMismatch() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val scenarios = listOf(
            Pair("f32", "f32"),
            Pair("i32", "f32"),
            Pair("u32", "f32"),
            Pair("vec2<f32>", "vec2<f32>"),
            Pair("vec3<f32>", "vec2<f32>"),
            Pair("vec2<f32>", "f32")
        )

        for (isAsync in booleanOptions) {
            for (scenario in scenarios) {
                val output = scenario.first
                val input = scenario.second

                val descriptor = GPURenderPipelineDescriptor(
                    layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                    vertex = getVertexStateWithOutputs(arrayOf("@location(0) @interpolate(flat, either) vout0: $output")),
                    fragment = getFragmentStateWithInputs(arrayOf("@location(0) @interpolate(flat, either) fin0: $input")),
                    primitive = GPUPrimitiveState(),
                    multisample = GPUMultisampleState()
                )

                val success = output == input
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }
}
