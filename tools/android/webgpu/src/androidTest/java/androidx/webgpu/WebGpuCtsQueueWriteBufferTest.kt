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
class WebGpuCtsQueueWriteBufferTest {

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
        runCatching { device.destroy() }
        webGpu.close()
    }

    private suspend fun tryWriteBuffer(success: Boolean, buffer: GPUBuffer, bufferOffset: Long, data: ByteBuffer) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            queue.writeBuffer(buffer, bufferOffset, data)
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
    fun testBufferStateValid() = runBlocking {
        val buffer = device.createBuffer(
            GPUBufferDescriptor(size = 16, usage = BufferUsage.CopyDst)
        )
        val data = ByteBuffer.allocateDirect(16)
        tryWriteBuffer(true, buffer, 0, data)
        runCatching { buffer.destroy() }
        Unit
    }

    @Test
    fun testBufferStateDestroyed() = runBlocking {
        val buffer = device.createBuffer(
            GPUBufferDescriptor(size = 16, usage = BufferUsage.CopyDst)
        )
        buffer.destroy()
        val data = ByteBuffer.allocateDirect(16)
        tryWriteBuffer(false, buffer, 0, data)
        Unit
    }

    @Test
    fun testBufferRanges() = runBlocking {
        val buffer = device.createBuffer(
            GPUBufferDescriptor(size = 16, usage = BufferUsage.CopyDst)
        )

        // Writing the full buffer
        val data16 = ByteBuffer.allocateDirect(16)
        tryWriteBuffer(true, buffer, 0, data16)

        // Writing out of bounds of buffer size
        val dataLg = ByteBuffer.allocateDirect(32)
        tryWriteBuffer(false, buffer, 0, dataLg)

        // Writing with offset that causes out of bounds
        val dataSm = ByteBuffer.allocateDirect(8)
        tryWriteBuffer(false, buffer, 12, dataSm)

        // Writing valid offset
        val data8 = ByteBuffer.allocateDirect(8)
        tryWriteBuffer(true, buffer, 8, data8)

        // Writing with an unaligned offset in the buffer (must be multiple of 4)
        val dataUnalignedOffset = ByteBuffer.allocateDirect(8)
        tryWriteBuffer(false, buffer, 3, dataUnalignedOffset)

        // Writing a buffer with unaligned size (must be multiple of 4)
        val dataUnalignedSize = ByteBuffer.allocateDirect(7)
        tryWriteBuffer(false, buffer, 0, dataUnalignedSize)

        runCatching { buffer.destroy() }
        Unit
    }

    @Test
    fun testBufferUsages() = runBlocking {
        // Without COPY_DST usage
        val buffer1 = device.createBuffer(
            GPUBufferDescriptor(size = 16, usage = BufferUsage.Storage)
        )
        tryWriteBuffer(false, buffer1, 0, ByteBuffer.allocateDirect(16))
        runCatching { buffer1.destroy() }

        // With COPY_DST usage
        val buffer2 = device.createBuffer(
            GPUBufferDescriptor(size = 16, usage = BufferUsage.Storage or BufferUsage.CopyDst)
        )
        tryWriteBuffer(true, buffer2, 0, ByteBuffer.allocateDirect(16))
        runCatching { buffer2.destroy() }

        Unit
    }
}
