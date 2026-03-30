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
class WebGpuCtsQueueDestroyedQuerySetTest {

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

    private suspend fun trySubmit(success: Boolean, commandBuffer: GPUCommandBuffer) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            queue.submit(arrayOf(commandBuffer))
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
    fun testResolveDestroyedQuerySet() = runBlocking {
        // Workaround for checking if occlusion is supported.
        // It's part of core features unless explicitly disabled.
        
        val querySet = runCatching {
            device.createQuerySet(
                GPUQuerySetDescriptor(
                    type = QueryType.Occlusion,
                    count = 1
                )
            )
        }.getOrNull()

        if (querySet == null) return@runBlocking // Skip if not supported on the specific adapter

        val buffer = device.createBuffer(
            GPUBufferDescriptor(size = 8, usage = BufferUsage.QueryResolve)
        )

        val encoder = device.createCommandEncoder()
        encoder.resolveQuerySet(querySet, 0, 1, buffer, 0L)
        val cb = encoder.finish()

        querySet.destroy() // Destroy before submit

        trySubmit(false, cb)

        runCatching { buffer.destroy() }
        Unit
    }

    @Test
    fun testResolveValidQuerySet() = runBlocking {
        val querySet = runCatching {
            device.createQuerySet(
                GPUQuerySetDescriptor(
                    type = QueryType.Occlusion,
                    count = 1
                )
            )
        }.getOrNull()

        if (querySet == null) return@runBlocking

        val buffer = device.createBuffer(
            GPUBufferDescriptor(size = 8, usage = BufferUsage.QueryResolve)
        )

        val encoder = device.createCommandEncoder()
        encoder.resolveQuerySet(querySet, 0, 1, buffer, 0L)
        val cb = encoder.finish()

        trySubmit(true, cb)

        runCatching { querySet.destroy() }
        runCatching { buffer.destroy() }
        Unit
    }
}
