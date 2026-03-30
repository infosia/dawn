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
class WebGpuCtsBufferInPassEncoderTest {

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

    enum class TestBufferUsage(val bufferType: Int, val isBindGroup: Boolean) {
        Uniform(BufferBindingType.Uniform, true),
        Storage(BufferBindingType.Storage, true),
        ReadOnlyStorage(BufferBindingType.ReadOnlyStorage, true),
        Vertex(0, false),
        Index(0, false)
    }

    private fun createBindGroupLayout(device: GPUDevice, usage: TestBufferUsage, visibility: Int): GPUBindGroupLayout {
        return device.createBindGroupLayout(
            GPUBindGroupLayoutDescriptor(
                entries = arrayOf(
                    GPUBindGroupLayoutEntry(binding = 0, visibility = visibility,
                        buffer = GPUBufferBindingLayout(type = usage.bufferType)
                    )
                )
            )
        )
    }

    private fun createBindGroup(device: GPUDevice, buffer: GPUBuffer, offset: Long, usage: TestBufferUsage, visibility: Int, size: Long = 256): GPUBindGroup {
        return device.createBindGroup(
            GPUBindGroupDescriptor(
                layout = createBindGroupLayout(device, usage, visibility),
                entries = arrayOf(
                    GPUBindGroupEntry(binding = 0, buffer = buffer, offset = offset, size = size)
                )
            )
        )
    }

    @Test
    fun testBufferUsageInOneComputePass_NoDispatch() = runBlocking {
        val usages = arrayOf(TestBufferUsage.Uniform, TestBufferUsage.Storage, TestBufferUsage.ReadOnlyStorage)
        for (u0 in usages) {
            for (u1 in usages) {
                device.pushErrorScope(ErrorFilter.Validation)

                val buffer = device.createBuffer(
                    GPUBufferDescriptor(
                        size = 512,
                        usage = BufferUsage.Uniform or BufferUsage.Storage
                    )
                )

                val encoder = device.createCommandEncoder()
                val pass = encoder.beginComputePass()

                pass.setBindGroup(0, createBindGroup(device, buffer, 0, u0, ShaderStage.Compute))
                pass.setBindGroup(1, createBindGroup(device, buffer, 0, u1, ShaderStage.Compute))
                pass.end()
                
                encoder.finish()
                val error = device.popErrorScope()
                // Never fails because no dispatch uses them.
                assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
            }
        }
    }

    @Test
    fun testBufferUsageInOneComputePass_OneDispatch() = runBlocking {
        val usages = arrayOf(TestBufferUsage.Uniform, TestBufferUsage.Storage, TestBufferUsage.ReadOnlyStorage)
        
        for (u0 in usages) {
            for (u1 in usages) {
                if (u0 == TestBufferUsage.Storage && u1 == TestBufferUsage.Storage) continue // writable binding aliasing is a separate issue

                val hasConflict = (u0 == TestBufferUsage.Storage) != (u1 == TestBufferUsage.Storage)

                device.pushErrorScope(ErrorFilter.Validation)

                val buffer = device.createBuffer(
                    GPUBufferDescriptor(
                        size = 512,
                        usage = BufferUsage.Uniform or BufferUsage.Storage
                    )
                )

                val encoder = device.createCommandEncoder()
                val pass = encoder.beginComputePass()

                val bgls = arrayOf(
                    createBindGroupLayout(device, u0, ShaderStage.Compute),
                    createBindGroupLayout(device, u1, ShaderStage.Compute)
                )
                val pipelineLayout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = bgls))
                val module = device.createShaderModule(GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = "@compute @workgroup_size(1) fn main() {}")))
                val pipeline = device.createComputePipeline(
                    GPUComputePipelineDescriptor(
                        layout = pipelineLayout,
                        compute = GPUComputeState(module = module, entryPoint = "main")
                    )
                )
                pass.setPipeline(pipeline)

                pass.setBindGroup(0, createBindGroup(device, buffer, 0, u0, ShaderStage.Compute))
                pass.setBindGroup(1, createBindGroup(device, buffer, 0, u1, ShaderStage.Compute))
                pass.dispatchWorkgroups(1)
                
                pass.end()
                encoder.finish()

                val error = device.popErrorScope()
                if (hasConflict) {
                    assertEquals(ErrorType.Validation, error)
                } else {
                    assertEquals(ErrorType.NoError,  error ?: ErrorType.NoError)
                }
            }
        }
    }

    @Test
    fun testBufferUsageInOneRenderPass_NoDraw() = runBlocking {
        val usages = arrayOf(TestBufferUsage.Uniform, TestBufferUsage.Storage, TestBufferUsage.ReadOnlyStorage, TestBufferUsage.Vertex, TestBufferUsage.Index)
        for (u0 in usages) {
            for (u1 in usages) {
                device.pushErrorScope(ErrorFilter.Validation)

                val buffer = device.createBuffer(
                    GPUBufferDescriptor(
                        size = 512,
                        usage = BufferUsage.Uniform or BufferUsage.Storage or BufferUsage.Vertex or BufferUsage.Index
                    )
                )

                val encoder = device.createCommandEncoder()
                val colorTex = device.createTexture(
                    GPUTextureDescriptor(
                        usage = TextureUsage.RenderAttachment,
                        size = GPUExtent3D(width = 1, height = 1, depthOrArrayLayers = 1),
                        format = TextureFormat.RGBA8Unorm
                    )
                )

                val pass = encoder.beginRenderPass(
                    GPURenderPassDescriptor(
                        colorAttachments = arrayOf(GPURenderPassColorAttachment(clearValue = GPUColor(0.0, 0.0, 0.0, 0.0), view = colorTex.createView(), loadOp = LoadOp.Load, storeOp = StoreOp.Store))
                    )
                )

                if (u0.isBindGroup) pass.setBindGroup(0, createBindGroup(device, buffer, 0, u0, ShaderStage.Fragment))
                else if (u0 == TestBufferUsage.Vertex) pass.setVertexBuffer(0, buffer, 0, 256)
                else if (u0 == TestBufferUsage.Index) pass.setIndexBuffer(buffer, IndexFormat.Uint16, 0, 256)

                if (u1.isBindGroup) pass.setBindGroup(1, createBindGroup(device, buffer, 0, u1, ShaderStage.Fragment))
                else if (u1 == TestBufferUsage.Vertex) pass.setVertexBuffer(1, buffer, 0, 256)
                else if (u1 == TestBufferUsage.Index) pass.setIndexBuffer(buffer, IndexFormat.Uint16, 0, 256)

                pass.end()
                encoder.finish()
                
                val fail = (u0 == TestBufferUsage.Storage) != (u1 == TestBufferUsage.Storage)

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
