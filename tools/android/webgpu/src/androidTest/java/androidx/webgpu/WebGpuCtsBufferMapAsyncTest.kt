package androidx.webgpu

import androidx.webgpu.helper.createWebGpu
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Validation tests for GPUBuffer.mapAsync.
 * Translated from third_party/webgpu-cts/src/webgpu/api/validation/buffer/mapping.spec.ts
 */
@RunWith(AndroidJUnit4::class)
class WebGpuCtsBufferMapAsyncTest {

    @get:Rule
    val apiLevelRule = ApiLevelSkipRule()

    private lateinit var device: GPUDevice
    private lateinit var webGpu: androidx.webgpu.helper.WebGpu

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        if (::webGpu.isInitialized) {
            webGpu.close()
        }
    }

    private fun createMappableBuffer(mode: Int, size: Long): GPUBuffer {
        val usage = if (mode == MapMode.Read) BufferUsage.MapRead else BufferUsage.MapWrite
        return device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts"))
    }

    /**
     * 1. Translated from mapAsync,usage
     * Test the usage validation for mapAsync.
     */
    @SmallTest
    @Test
    fun testMapAsyncUsage() = runBlocking {
        val cases = listOf(
            Triple(MapMode.Read, BufferUsage.MapRead, true),
            Triple(MapMode.Write, BufferUsage.MapWrite, true),
            // Combining invalid usages
            Triple(MapMode.Read, BufferUsage.MapWrite, false),
            Triple(MapMode.Write, BufferUsage.MapRead, false)
        )

        for (case in cases) {
            val mapMode = case.first
            val usage = case.second
            val isValid = case.third

            val buffer = device.createBuffer(GPUBufferDescriptor(size = 16, usage = usage, label = "cts"))

            if (isValid) {
                buffer.mapAndAwait(mapMode, 0, 16)
                buffer.unmap()
            } else {
                val result = runCatching {
                    buffer.mapAndAwait(mapMode, 0, 16)
                }
                org.junit.Assert.assertTrue(result.isFailure)
            }
        }
    }

    /**
     * 2. Translated from mapAsync,state,destroyed
     * Test that mapAsync is an error when called on a destroyed buffer.
     */
    @SmallTest
    @Test
    fun testMapAsyncStateDestroyed() = runBlocking {
        val mapModes = listOf(MapMode.Read, MapMode.Write)

        for (mapMode in mapModes) {
            val buffer = createMappableBuffer(mapMode, 16)
            buffer.destroy()

            val result = runCatching {
                buffer.mapAndAwait(mapMode, 0, 16)
            }
            org.junit.Assert.assertTrue(result.isFailure)
        }
    }

    /**
     * 3. Translated from mapAsync,state,mappedAtCreation
     * Test mapAsync fails on mappedAtCreation buffer.
     */
    @SmallTest
    @Test
    fun testMapAsyncStateMappedAtCreation() = runBlocking {
        val cases = listOf(
            Pair(MapMode.Read, BufferUsage.MapRead),
            Pair(MapMode.Write, BufferUsage.MapWrite)
        )

        for (case in cases) {
            val mapMode = case.first
            val usage = case.second

            val buffer = device.createBuffer(GPUBufferDescriptor(
                size = 16, 
                usage = usage, 
                mappedAtCreation = true, 
                label = "cts"
            ))

            // map on already mappedAtCreation should fail synchronously in JS style pre-validate
            val result = runCatching {
                buffer.mapAndAwait(mapMode, 0, 16)
            }
            org.junit.Assert.assertTrue(result.isFailure)

            buffer.unmap()
            buffer.mapAndAwait(mapMode, 0, 16) // Should now succeed
            buffer.unmap()
        }
    }

    /**
     * 4. Translated from mapAsync,state,mapped
     * Test mapAsync fails on fully mapped buffer.
     */
    @SmallTest
    @Test
    fun testMapAsyncStateMapped() = runBlocking {
        val mapModes = listOf(MapMode.Read, MapMode.Write)

        for (mapMode in mapModes) {
            val buffer = createMappableBuffer(mapMode, 16)
            buffer.mapAndAwait(mapMode, 0, 16)

            val result = runCatching {
                buffer.mapAndAwait(mapMode, 0, 16)
            }
            org.junit.Assert.assertTrue(result.isFailure)

            buffer.unmap()
            buffer.mapAndAwait(mapMode, 0, 16) // allowed again
            buffer.unmap()
        }
    }

    /**
     * 5. Translated from mapAsync,sizeUnspecifiedOOB
     * Test offset <= size boundaries.
     */
    @SmallTest
    @Test
    fun testMapAsyncSizeUnspecifiedOOB() = runBlocking {
        val bufferSize = 16L
        val offsets = listOf(0L, 8L, 16L, 24L) // 24 > size

        for (offset in offsets) {
            val buffer = createMappableBuffer(MapMode.Read, bufferSize)
            val isValid = offset <= bufferSize

            if (isValid) {
                buffer.mapAndAwait(MapMode.Read, offset, bufferSize - offset)
                buffer.unmap()
            } else {
                val result = runCatching {
                    buffer.mapAndAwait(MapMode.Read, offset, 0)
                }
                org.junit.Assert.assertTrue(result.isFailure)
            }
        }
    }

    /**
     * 6. Translated from mapAsync,offsetAndSizeAlignment
     * Test mapAsync alignment requirements (offset multiple of 8, size multiple of 4).
     */
    @SmallTest
    @Test
    fun testMapAsyncOffsetAndSizeAlignment() = runBlocking {
        val offsets = listOf(0L, 8L, 4L) // 8 offset alignment
        val sizes = listOf(0L, 4L, 2L)   // 4 size alignment

        for (offset in offsets) {
            for (size in sizes) {
                val buffer = createMappableBuffer(MapMode.Read, 16)
                val isValid = offset % 8L == 0L && size % 4L == 0L

                if (isValid) {
                    buffer.mapAndAwait(MapMode.Read, offset, size)
                    buffer.unmap()
                } else {
                    val result = runCatching {
                        buffer.mapAndAwait(MapMode.Read, offset, size)
                    }
                    org.junit.Assert.assertTrue(result.isFailure)
                }
            }
        }
    }
}
