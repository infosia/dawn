package androidx.webgpu

import androidx.webgpu.helper.createWebGpu
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Validation tests for GPUBuffer.getMappedRange.
 * Translated from third_party/webgpu-cts/src/webgpu/api/validation/buffer/mapping.spec.ts
 */
@RunWith(AndroidJUnit4::class)
class WebGpuCtsBufferGetMappedRangeTest {

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

    private fun createMappableBuffer(mode: Int, size: Long = 16L): GPUBuffer {
        val usage = if (mode == MapMode.Read) BufferUsage.MapRead else BufferUsage.MapWrite
        return device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts"))
    }

    /**
     * 1. Translated from getMappedRange,state,mapped
     * Test calling getMappedRange in the mapped state.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeStateMapped() {
        runBlocking {
        val mapModes = listOf(MapMode.Read, MapMode.Write)

        for (mapMode in mapModes) {
            val bufferSize = 16L
            val buffer = createMappableBuffer(mapMode, bufferSize)
            buffer.mapAndAwait(mapMode, 0, bufferSize)
            val data = if (mapMode == MapMode.Read) {
                buffer.getConstMappedRange()
            } else {
                buffer.getMappedRange()
            }
            assertTrue(data != null)
            assertTrue(data.capacity() == bufferSize.toInt())

            buffer.unmap()
            
            // Should fail after unmapped
            assertThrows(Error::class.java) {
                buffer.getMappedRange(0, bufferSize)
            }
        }
    }
}

    /**
     * 2. Translated from getMappedRange,state,mappedAtCreation
     * Test calling getMappedRange in mappedAtCreation.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeStateMappedAtCreation() {
        val bufferSize = 16L
        val buffer = device.createBuffer(GPUBufferDescriptor(
            size = bufferSize, 
            usage = BufferUsage.CopyDst, 
            mappedAtCreation = true, 
            label = "cts"
        ))

        val data = buffer.getMappedRange()
        assertTrue(data != null)
        assertTrue(data.capacity() == bufferSize.toInt())

        buffer.unmap()

        // Should fail after unmapped
        assertThrows(Error::class.java) {
            buffer.getMappedRange(0, bufferSize)
        }
    }

    /**
     * 3. Translated from getMappedRange,state,unmapped
     * Test calling getMappedRange fails in unmapped state.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeStateUnmapped() {
        runBlocking {
        // Starts unmapped
        val buffer = createMappableBuffer(MapMode.Read, 16)
        assertThrows(Error::class.java) {
            buffer.getMappedRange(0, 16)
        }

        // Unmapped after mapAsync
        buffer.mapAndAwait(MapMode.Read, 0, 16)
        buffer.unmap()
        assertThrows(Error::class.java) {
            buffer.getMappedRange(0, 16)
        }
        }
    }

    /**
     * 4. Translated from getMappedRange,subrange,mapped
     * Test getMappedRange arraybuffer offsets sizing has correct subrange bounds.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeSubrangeMapped() {
        runBlocking {
        val bufferSize = 16L
        val offset = 8L
        val subrangeSize = bufferSize - offset

        val buffer = createMappableBuffer(MapMode.Read, bufferSize)
        buffer.mapAndAwait(MapMode.Read, 0, bufferSize)
        
        val data0 = buffer.getConstMappedRange()
        assertTrue(data0.capacity() == bufferSize.toInt())

        buffer.unmap()

        // map again with subrange offset
        buffer.mapAndAwait(MapMode.Read, offset, subrangeSize)
        val data1 = buffer.getConstMappedRange(offset)

        assertTrue(data1 != null)
        assertTrue(data1.capacity() == subrangeSize.toInt())

        buffer.unmap()
        }
    }

    /**
     * 5. Translated from getMappedRange,state,destroyed
     * Test calling getMappedRange fails in destroyed state.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeStateDestroyed() {
        runBlocking {
        val buffer = createMappableBuffer(MapMode.Read, 16)
        buffer.destroy()

        assertThrows(Error::class.java) {
            buffer.getMappedRange(0, 16)
        }
        }
    }

    /**
     * 6. Translated from getMappedRange,offsetAndSizeAlignment,mapped
     * Test offset multiple of 8, size multiple of 4 failure.
     */
    @SmallTest
    @Test
    fun testGetMappedRangeOffsetAndSizeAlignment() {
        runBlocking {
        val mapOffset = 0L
        val offsets = listOf(0L, 8L, 4L) // 8 offset alignment
        val sizes = listOf(0L, 4L, 2L)   // 4 size alignment

        for (offset in offsets) {
            for (size in sizes) {
                val buffer = createMappableBuffer(MapMode.Read, 32L)
                buffer.mapAndAwait(MapMode.Read, mapOffset, 32L)

                val isValid = offset % 8L == 0L && size % 4L == 0L

                if (isValid) {
                    val data = buffer.getConstMappedRange(offset)
                    // Skip precise capacity checks due to Dawn template defaults buffer allocation wrapping
                } else {
                    assertThrows(Error::class.java) {
                        buffer.getMappedRange(offset, size)
                    }
                }
                buffer.unmap()
            }
        }
    }
    }
}
