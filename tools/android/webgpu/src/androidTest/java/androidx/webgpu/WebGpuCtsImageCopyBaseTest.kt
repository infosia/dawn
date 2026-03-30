package androidx.webgpu

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import java.nio.ByteBuffer

abstract class WebGpuCtsImageCopyBaseTest {
    protected lateinit var webGpu: WebGpu
    protected lateinit var device: GPUDevice

    @Before
    open fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    open fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    enum class Method {
        WriteTexture, CopyB2T, CopyT2B
    }

    protected fun testBuffer(
        buffer: GPUBuffer,
        texture: GPUTexture,
        textureDataLayout: GPUTexelCopyBufferLayout,
        size: GPUExtent3D,
        method: Method,
        dataSize: Long,
        success: Boolean
    ) = runBlocking {
        var errorThrown = false

        try {
            device.pushErrorScope(ErrorFilter.Validation)
            when (method) {
                Method.WriteTexture -> {
                    val dummyData = ByteBuffer.allocateDirect(dataSize.toInt())
                    device.queue.writeTexture(
                        GPUTexelCopyTextureInfo(texture = texture),
                        dummyData,
                        size,
                        textureDataLayout
                    )
                }
                Method.CopyB2T -> {
                    val encoder = device.createCommandEncoder()
                    encoder.copyBufferToTexture(
                        GPUTexelCopyBufferInfo(buffer = buffer, layout = textureDataLayout),
                        GPUTexelCopyTextureInfo(texture = texture),
                        size
                    )
                    val cmd = encoder.finish()
                    device.queue.submit(arrayOf(cmd))
                }
                Method.CopyT2B -> {
                    val encoder = device.createCommandEncoder()
                    encoder.copyTextureToBuffer(
                        GPUTexelCopyTextureInfo(texture = texture),
                        GPUTexelCopyBufferInfo(buffer = buffer, layout = textureDataLayout),
                        size
                    )
                    val cmd = encoder.finish()
                    device.queue.submit(arrayOf(cmd))
                }
            }
            
            val error = device.popErrorScope()
            if (error != ErrorType.NoError) {
                errorThrown = true
            }
        } catch (e: Exception) {
            errorThrown = true
        }

        if (success) {
            assert(!errorThrown) { "Expected success for method $method, but caught Validation Error." }
        } else {
            assert(errorThrown) { "Expected validation error for method $method, but the operation succeeded." }
        }
    }
}
