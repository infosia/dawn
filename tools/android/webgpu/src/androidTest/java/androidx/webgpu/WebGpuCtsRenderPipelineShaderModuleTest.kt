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
class WebGpuCtsRenderPipelineShaderModuleTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    private lateinit var mismatchedWebGpu: WebGpu
    private lateinit var mismatchedDevice: GPUDevice

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device

        mismatchedWebGpu = createWebGpu()
        mismatchedDevice = mismatchedWebGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()

        runCatching { mismatchedDevice.destroy() }
        mismatchedWebGpu.close()
    }

    @Test
    fun testDeviceMismatch() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val mismatchScenarios = arrayOf(
            Triple(false, false, true), // vertex=false, fragment=false, success=true
            Triple(true, false, false), // vertex=true, fragment=false, success=false
            Triple(false, true, false)  // vertex=false, fragment=true, success=false
        )

        for (isAsync in booleanOptions) {
            for (scenario in mismatchScenarios) {
                val vertexMismatched = scenario.first
                val fragmentMismatched = scenario.second
                val success = scenario.third

                val vertexDev = if (vertexMismatched) mismatchedDevice else device
                val fragmentDev = if (fragmentMismatched) mismatchedDevice else device

                // It's possible for createShaderModule to fail if the device is mismatched during creation?
                // Actually, the test says "createRenderPipeline cannot be called with a shader module created from another device".
                // So creating the modules is fine.
                val vertexModule = vertexDev.createShaderModule(
                    GPUShaderModuleDescriptor(
                        shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultVertexShaderCode)
                    )
                )

                val fragmentModule = fragmentDev.createShaderModule(
                    GPUShaderModuleDescriptor(
                        shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode)
                    )
                )

                val descriptor = GPURenderPipelineDescriptor(
                    layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf())),
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

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, success, { descriptor })
            }
        }
    }

    private fun createInvalidShaderModule(dev: GPUDevice): GPUShaderModule {
        dev.pushErrorScope(ErrorFilter.Validation)
        val invalidModule = dev.createShaderModule(
            GPUShaderModuleDescriptor(
                shaderSourceWGSL = GPUShaderSourceWGSL(code = "invalid wgsl code syntax error!!!")
            )
        )
        // Consume the expected validation error so it doesn't fail the pipeline test scope later!
        runBlocking { 
            try {
                dev.popErrorScope() 
            } catch (e: Exception) {
                // Expected
            }
        }
        return invalidModule
    }

    @Test
    fun testInvalidVertex() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (isVertexShaderValid in booleanOptions) {
                val vertexModule = if (isVertexShaderValid) {
                    device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultVertexShaderCode)
                        )
                    )
                } else {
                    createInvalidShaderModule(device)
                }

                val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                    device,
                    vertexState = GPUVertexState(
                        module = vertexModule,
                        entryPoint = "main",
                        buffers = arrayOf()
                    )
                )

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, isVertexShaderValid, { descriptor })
            }
        }
    }

    @Test
    fun testInvalidFragment() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (isFragmentShaderValid in booleanOptions) {
                val fragmentModule = if (isFragmentShaderValid) {
                    device.createShaderModule(
                        GPUShaderModuleDescriptor(
                            shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode)
                        )
                    )
                } else {
                    createInvalidShaderModule(device)
                }

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
                        module = fragmentModule,
                        entryPoint = "main",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    ),
                    primitive = GPUPrimitiveState(topology = PrimitiveTopology.TriangleList),
                    multisample = GPUMultisampleState()
                )

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, isFragmentShaderValid, { descriptor })
            }
        }
    }
}
