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
class WebGpuCtsRenderPipelineVertexStateTest {

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
    fun testMaxVertexBuffers() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val maxBuffers = device.getLimits().maxVertexBuffers

        val limitsToTest = intArrayOf(maxBuffers, maxBuffers + 1)

        for (isAsync in booleanOptions) {
            for (limit in limitsToTest) {
                
                val buffers = mutableListOf<GPUVertexBufferLayout>()
                for (i in 0 until limit) {
                    buffers.add(GPUVertexBufferLayout(
                        arrayStride = 4,
                        attributes = arrayOf(GPUVertexAttribute(format = VertexFormat.Float32, offset = 0, shaderLocation = i))
                    ))
                }

                var shaderAttrs = ""
                for (i in 0 until limit) {
                    shaderAttrs += "@location($i) attr$i: f32,\n"
                }

                val vertexCode = """
                    struct VertexIn {
                        $shaderAttrs
                    }
                    @vertex fn main(vin: VertexIn) -> @builtin(position) vec4<f32> {
                        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
                    }
                """.trimIndent()

                val bgl = device.createBindGroupLayout(GPUBindGroupLayoutDescriptor(entries = arrayOf()))
                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bgl)))

                val descriptor = GPURenderPipelineDescriptor(
                    layout = pipelineLayout,
                    vertex = GPUVertexState(
                        module = device.createShaderModule(
                            GPUShaderModuleDescriptor(
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = vertexCode)
                            )
                        ),
                        entryPoint = "main",
                        buffers = buffers.toTypedArray()
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

                val success = limit <= maxBuffers
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    @Test
    fun testMaxVertexAttributes() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val maxAttributes = device.getLimits().maxVertexAttributes

        val limitsToTest = intArrayOf(maxAttributes, maxAttributes + 1)

        for (isAsync in booleanOptions) {
            for (limit in limitsToTest) {
                
                val attributes = mutableListOf<GPUVertexAttribute>()
                for (i in 0 until limit) {
                    attributes.add(GPUVertexAttribute(format = VertexFormat.Float32, offset = (i * 4).toLong(), shaderLocation = i))
                }

                val buffer = GPUVertexBufferLayout(
                    arrayStride = (limit * 4).toLong(),
                    attributes = attributes.toTypedArray()
                )

                var shaderAttrs = ""
                for (i in 0 until limit) {
                    shaderAttrs += "@location($i) attr$i: f32,\n"
                }

                val vertexCode = """
                    struct VertexIn {
                        $shaderAttrs
                    }
                    @vertex fn main(vin: VertexIn) -> @builtin(position) vec4<f32> {
                        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
                    }
                """.trimIndent()

                val bgl = device.createBindGroupLayout(GPUBindGroupLayoutDescriptor(entries = arrayOf()))
                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf(bgl)))

                val descriptor = GPURenderPipelineDescriptor(
                    layout = pipelineLayout,
                    vertex = GPUVertexState(
                        module = device.createShaderModule(
                            GPUShaderModuleDescriptor(
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = vertexCode)
                            )
                        ),
                        entryPoint = "main",
                        buffers = arrayOf(buffer)
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

                val success = limit <= maxAttributes
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    @Test
    fun testArrayStrideMaximum() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val limit = device.getLimits().maxVertexBufferArrayStride.toLong()

        val stridesToTest = longArrayOf(limit, limit + 4)

        for (isAsync in booleanOptions) {
            for (stride in stridesToTest) {
                
                val buffer = GPUVertexBufferLayout(
                    arrayStride = stride,
                    attributes = arrayOf(GPUVertexAttribute(format = VertexFormat.Float32, offset = 0, shaderLocation = 0))
                )

                val descriptor = GPURenderPipelineDescriptor(
                    layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
                    vertex = GPUVertexState(
                        module = device.createShaderModule(
                            GPUShaderModuleDescriptor(
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultVertexShaderCode)
                            )
                        ),
                        entryPoint = "main",
                        buffers = arrayOf(buffer)
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

                val success = stride <= limit
                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }
}
