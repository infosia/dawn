package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsQueueSubmitTest {

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

    private suspend fun trySubmit(success: Boolean, commandBuffers: Array<GPUCommandBuffer>) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            queue.submit(commandBuffers)
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
    fun testDuplicateBuffers() = runBlocking {
        val encoder = device.createCommandEncoder()
        val cb = encoder.finish()

        trySubmit(false, arrayOf(cb, cb))
    }

    @Test
    fun testSubmitInvalidates() = runBlocking {
        val encoder = device.createCommandEncoder()
        val cb = encoder.finish()

        // First submit should succeed
        trySubmit(true, arrayOf(cb))

        // Second submit of the same buffer should fail
        trySubmit(false, arrayOf(cb))
    }
}
