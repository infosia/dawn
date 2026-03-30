package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking
import org.junit.Ignore

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@Ignore("TODO: Framework Bug. popErrorScope() throws ValidationException instead of returning ErrorType.Validation. Fix generator/templates/art/api_kotlin_callback.kt to call callback.onResult(type) instead of callback.onError() when type != NoError.")
@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsBufferInPassMiscTest {

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

    enum class TestBufferUsage {
        Sampled,
        Storage,
        Uniform,
        Vertex,
        Index,
        Indirect,
        IndexedIndirect
    }

    private fun getBufferUsage(usage: TestBufferUsage): Int {
        return when (usage) {
            TestBufferUsage.Sampled -> BufferUsage.Uniform or BufferUsage.Storage
            TestBufferUsage.Storage -> BufferUsage.Storage
            TestBufferUsage.Uniform -> BufferUsage.Uniform
            TestBufferUsage.Vertex -> BufferUsage.Vertex
            TestBufferUsage.Index -> BufferUsage.Index
            TestBufferUsage.Indirect, TestBufferUsage.IndexedIndirect -> BufferUsage.Indirect
        }
    }

    @Test
    fun testResetBufferUsageBeforeDispatch() = runBlocking {
        val usageOptions = arrayOf(TestBufferUsage.Storage, TestBufferUsage.Uniform, TestBufferUsage.Indirect)

        for (u0 in usageOptions) {
            for (u1 in usageOptions) {
                device.pushErrorScope(ErrorFilter.Validation)

                val buffer0 = device.createBuffer(
                    GPUBufferDescriptor(
                        usage = getBufferUsage(u0) or getBufferUsage(u1),
                        size = 256
                    )
                )

                val encoder = device.createCommandEncoder()

                val computeEncoder = encoder.beginComputePass()

                val bindGroupLayouts = mutableListOf<GPUBindGroupLayout>()

                val bglDef = GPUBindGroupLayoutDescriptor(entries = arrayOf(
                    if (u0 == TestBufferUsage.Uniform) GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Compute, buffer = GPUBufferBindingLayout(type = BufferBindingType.Uniform))
                    else GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Compute, buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage))
                ))
                val bg0Layout = device.createBindGroupLayout(bglDef)
                bindGroupLayouts.add(bg0Layout)
                        
                val bg0 = device.createBindGroup(GPUBindGroupDescriptor(layout = bg0Layout, entries = arrayOf(
                    GPUBindGroupEntry(binding = 0, buffer = buffer0)
                )))

                when (u0) {
                    TestBufferUsage.Storage, TestBufferUsage.Uniform -> {
                        computeEncoder.setBindGroup(0, bg0)
                    }
                    else -> {}
                }

                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = bindGroupLayouts.toTypedArray()))
                val computeModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = "@compute @workgroup_size(1) fn main() {}")))
                val pipeline = device.createComputePipeline(GPUComputePipelineDescriptor(layout = pipelineLayout, compute = GPUComputeState(module = computeModule, entryPoint = "main")))

                computeEncoder.setPipeline(pipeline)

                when (u1) {
                    TestBufferUsage.Indirect -> {
                        computeEncoder.dispatchWorkgroupsIndirect(buffer0, 0)
                    }
                    TestBufferUsage.Storage, TestBufferUsage.Uniform -> {
                        computeEncoder.dispatchWorkgroups(1)
                    }
                    else -> {}
                }

                computeEncoder.end()
                encoder.finish()

                val error = device.popErrorScope()
                val fail = (u0 == TestBufferUsage.Storage && u1 == TestBufferUsage.Indirect)
                if (fail) {
                    assertEquals(ErrorType.Validation, error)
                } else {
                    assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
                }
            }
        }
    }

    @Test
    fun testResetBufferUsageBeforeDraw() = runBlocking {
        val usageOptions = TestBufferUsage.values()

        for (u0 in usageOptions) {
            for (u1 in usageOptions) {
                device.pushErrorScope(ErrorFilter.Validation)

                val buffer0 = device.createBuffer(
                    GPUBufferDescriptor(
                        usage = getBufferUsage(u0) or getBufferUsage(u1),
                        size = 256
                    )
                )

                val encoder = device.createCommandEncoder()
                val colorTexture = device.createTexture(
                    GPUTextureDescriptor(
                        usage = TextureUsage.RenderAttachment,
                        size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                        format = TextureFormat.RGBA8Unorm
                    )
                )

                val passDescriptor = GPURenderPassDescriptor(colorAttachments = arrayOf(
                    GPURenderPassColorAttachment(clearValue = GPUColor(0.0, 0.0, 0.0, 0.0), view = colorTexture.createView(), loadOp = LoadOp.Clear, storeOp = StoreOp.Store)
                ))

                val renderEncoder = encoder.beginRenderPass(passDescriptor)

                val bindGroupLayouts = mutableListOf<GPUBindGroupLayout>()

                val bglDef = GPUBindGroupLayoutDescriptor(entries = arrayOf(
                    if (u0 == TestBufferUsage.Uniform) GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment, buffer = GPUBufferBindingLayout(type = BufferBindingType.Uniform))
                    else GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment, buffer = GPUBufferBindingLayout(type = BufferBindingType.ReadOnlyStorage))
                ))
                
                val bg0Layout = device.createBindGroupLayout(bglDef)
                if (u0 == TestBufferUsage.Uniform || u0 == TestBufferUsage.Storage) {
                    bindGroupLayouts.add(bg0Layout)
                }
                
                var vertexBufferCount = 0

                when (u0) {
                    TestBufferUsage.Storage, TestBufferUsage.Uniform -> {
                        val bg0 = device.createBindGroup(GPUBindGroupDescriptor(layout = bg0Layout, entries = arrayOf(
                            GPUBindGroupEntry(binding = 0, buffer = buffer0)
                        )))
                        renderEncoder.setBindGroup(0, bg0)
                    }
                    TestBufferUsage.Vertex -> {
                        renderEncoder.setVertexBuffer(0, buffer0, 0, 256)
                        vertexBufferCount = 1
                    }
                    TestBufferUsage.Index -> {
                        renderEncoder.setIndexBuffer(buffer0, IndexFormat.Uint16, 0, 256)
                    }
                    else -> {}
                }

                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = bindGroupLayouts.toTypedArray()))
                val vertexModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = "@vertex fn main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0, 0.0, 0.0, 1.0); }")))
                val fragmentModule = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = "@fragment fn main() -> @location(0) vec4<f32> { return vec4<f32>(0.0, 0.0, 0.0, 1.0); }")))
                
                val vertexBuffers = mutableListOf<GPUVertexBufferLayout>()
                for (i in 0 until vertexBufferCount) {
                    vertexBuffers.add(GPUVertexBufferLayout(
                        arrayStride = 4,
                        attributes = arrayOf(GPUVertexAttribute(format = VertexFormat.Float32, offset = 0, shaderLocation = i))
                    ))
                }
                
                val renderPipeline = device.createRenderPipeline(
                    GPURenderPipelineDescriptor(
                        layout = pipelineLayout,
                        vertex = GPUVertexState(module = vertexModule, entryPoint = "main", buffers = vertexBuffers.toTypedArray()),
                        fragment = GPUFragmentState(module = fragmentModule, entryPoint = "main", targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))),
                        primitive = GPUPrimitiveState(topology = PrimitiveTopology.PointList)
                    )
                )
                renderEncoder.setPipeline(renderPipeline)

                when (u1) {
                    TestBufferUsage.IndexedIndirect -> {
                        if (u0 != TestBufferUsage.Index) {
                            val indexBuffer = device.createBuffer(GPUBufferDescriptor(usage = BufferUsage.Index, size = 4))
                            renderEncoder.setIndexBuffer(indexBuffer, IndexFormat.Uint16, 0, 4)
                        }
                        renderEncoder.drawIndexedIndirect(buffer0, 0)
                    }
                    TestBufferUsage.Indirect -> {
                        renderEncoder.drawIndirect(buffer0, 0)
                    }
                    TestBufferUsage.Index -> {
                        if (u0 != TestBufferUsage.Index) {
                            renderEncoder.setIndexBuffer(buffer0, IndexFormat.Uint16, 0, 256)
                        }
                        renderEncoder.drawIndexed(1)
                    }
                    else -> {
                        renderEncoder.draw(1)
                    }
                }

                renderEncoder.end()
                encoder.finish()

                val fail = (u0 == TestBufferUsage.Storage && (u1 == TestBufferUsage.Index || u1 == TestBufferUsage.Indirect || u1 == TestBufferUsage.IndexedIndirect)) ||
                           (u0 == TestBufferUsage.Vertex && u1 == TestBufferUsage.Index) ||
                           (u0 == TestBufferUsage.Index && u1 == TestBufferUsage.Vertex)
                val error = device.popErrorScope()
                if (fail) {
                    assertEquals(ErrorType.Validation, error)
                } else {
                    assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
                }
            }
        }
    }
}
