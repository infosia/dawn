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
class WebGpuCtsRenderPipelineMiscTest {

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
    fun testBasic() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        for (isAsync in booleanOptions) {
            val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(device)
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, true, { descriptor })
        }
    }

    @Test
    fun testNoAttachment() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        for (isAsync in booleanOptions) {
            val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                device,
                noFragment = true,
                depthStencil = null
            )
            WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, false, { descriptor })
        }
    }

    @Test
    fun testVertexStateOnly() = runBlocking {
        val booleanOptions = arrayOf(true, false)
        val depthFormats = arrayOf(
            TextureFormat.Depth24Plus,
            TextureFormat.Depth24PlusStencil8,
            TextureFormat.Depth32Float,
            TextureFormat.Undefined
        )

        for (isAsync in booleanOptions) {
            for (format in depthFormats) {
                for (hasColor in booleanOptions) {
                    if (!hasColor && format == TextureFormat.Undefined) {
                        continue // Render pipeline needs at least one attachment
                    }

                    val depthStencilState = if (format == TextureFormat.Undefined) {
                        null
                    } else {
                        GPUDepthStencilState(
                            format = format,
                            depthWriteEnabled = 0, // 0 is OptionalBool.False, 1 is True
                            depthCompare = CompareFunction.Always
                        )
                    }

                    val targets = if (hasColor) arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm)) else arrayOf()

                    val descriptor = WebGpuCtsRenderPipelineHelper.getDescriptor(
                        device,
                        noFragment = true,
                        depthStencil = depthStencilState,
                        targets = targets
                    )

                    WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, depthStencilState != null, { descriptor })
                }
            }
        }
    }

    @Test
    fun testDeviceMismatch() = runBlocking {
        val booleanOptions = arrayOf(true, false)

        for (isAsync in booleanOptions) {
            for (mismatched in booleanOptions) {
                val sourceDevice = if (mismatched) mismatchedDevice else device
                val layout = sourceDevice.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf()))

                val descriptor = GPURenderPipelineDescriptor(
                    layout = layout,
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
                                shaderSourceWGSL = GPUShaderSourceWGSL(code = WebGpuCtsRenderPipelineHelper.kDefaultFragmentShaderCode)
                            )
                        ),
                        entryPoint = "main",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    ),
                    primitive = GPUPrimitiveState(),
                    multisample = GPUMultisampleState()
                )

                WebGpuCtsRenderPipelineHelper.doCreateRenderPipelineTest(device, isAsync, !mismatched, { descriptor })
            }
        }
    }
}
