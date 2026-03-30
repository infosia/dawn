package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking
import java.nio.ByteBuffer

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsQueueBufferMappedTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice
    private lateinit var queue: GPUQueue

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
        queue = device.queue
    }

    @After
    fun tearDown() {
        runCatching { queue.close() }
        runCatching { device.destroy() }
        webGpu.close()
    }

    private suspend fun runBufferDependencyTest(usage: Int, callback: (GPUBuffer) -> Unit) {
        val mapMode = if ((usage and BufferUsage.MapRead) != 0) MapMode.Read else MapMode.Write

        // Create a mappable buffer, and one that will remain unmapped for comparison.
        val mappableBuffer = device.createBuffer(
            GPUBufferDescriptor(size = 8, usage = usage, mappedAtCreation = false)
        )
        val unmappedBuffer = device.createBuffer(
            GPUBufferDescriptor(size = 8, usage = usage, mappedAtCreation = false)
        )

        // Run the given operation before the buffer is mapped. Should succeed!
        tryCallbackAndExpectSuccess(unmappedBuffer, true, callback)

        // Map the buffer
        mappableBuffer.mapAndAwait(mapMode, 0L, 8L)

        // Run the given operation while the buffer is in the process of mapping or is mapped. Should fail.
        tryCallbackAndExpectSuccess(mappableBuffer, false, callback)

        // Unmap and run. Should succeed.
        mappableBuffer.unmap()
        tryCallbackAndExpectSuccess(mappableBuffer, true, callback)

        // Mapped at creation should also fail
        val mappedAtCreationBuffer = device.createBuffer(
            GPUBufferDescriptor(size = 8, usage = usage, mappedAtCreation = true)
        )
        tryCallbackAndExpectSuccess(mappedAtCreationBuffer, false, callback)
        mappedAtCreationBuffer.unmap()
        tryCallbackAndExpectSuccess(mappedAtCreationBuffer, true, callback)
        
        runCatching { mappableBuffer.destroy() }
        runCatching { unmappedBuffer.destroy() }
        runCatching { mappedAtCreationBuffer.destroy() }
    }

    private suspend fun tryCallbackAndExpectSuccess(buffer: GPUBuffer, success: Boolean, callback: (GPUBuffer) -> Unit) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            callback(buffer)
            device.popErrorScope()
            if (!success) {
                fail("Expected validation error but succeeded")
            }
        } catch (e: Exception) {
            if (success) {
                fail("Expected success but failed with: $e")
            }
        }
    }

    @Test
    fun testWriteBuffer() = runBlocking {
        runBufferDependencyTest(BufferUsage.MapRead or BufferUsage.CopyDst) { buffer ->
            val data = ByteBuffer.allocateDirect(4)
            queue.writeBuffer(buffer, 0, data)
        }
        Unit
    }

    @Test
    fun testCopyBufferToBufferDst() = runBlocking {
        val sourceBuffer = device.createBuffer(GPUBufferDescriptor(size = 8, usage = BufferUsage.CopySrc))
        runBufferDependencyTest(BufferUsage.MapRead or BufferUsage.CopyDst) { buffer ->
            val commandEncoder = device.createCommandEncoder()
            commandEncoder.copyBufferToBuffer(sourceBuffer, 0, buffer, 0, 4)
            queue.submit(arrayOf(commandEncoder.finish()))
        }
        runCatching { sourceBuffer.destroy() }
        Unit
    }

    @Test
    fun testCopyBufferToTexture() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(1, 1, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopyDst
            )
        )
        runBufferDependencyTest(BufferUsage.MapWrite or BufferUsage.CopySrc) { buffer ->
            val commandEncoder = device.createCommandEncoder()
            commandEncoder.copyBufferToTexture(
                GPUTexelCopyBufferInfo(buffer = buffer),
                GPUTexelCopyTextureInfo(texture = texture),
                GPUExtent3D(1, 1, 1)
            )
            queue.submit(arrayOf(commandEncoder.finish()))
        }
        runCatching { texture.destroy() }
        Unit
    }

    @Test
    fun testCopyTextureToBuffer() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(1, 1, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopySrc
            )
        )
        runBufferDependencyTest(BufferUsage.MapRead or BufferUsage.CopyDst) { buffer ->
            val commandEncoder = device.createCommandEncoder()
            commandEncoder.copyTextureToBuffer(
                GPUTexelCopyTextureInfo(texture = texture),
                GPUTexelCopyBufferInfo(buffer = buffer),
                GPUExtent3D(1, 1, 1)
            )
            queue.submit(arrayOf(commandEncoder.finish()))
        }
        runCatching { texture.destroy() }
        Unit
    }
}
