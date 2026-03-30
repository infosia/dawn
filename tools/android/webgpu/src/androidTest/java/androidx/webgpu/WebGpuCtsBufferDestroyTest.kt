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
 * Validation tests for GPUBuffer.destroy.
 * Translated from third_party/webgpu-cts/src/webgpu/api/validation/buffer/destroy.spec.ts
 */
@RunWith(AndroidJUnit4::class)
class WebGpuCtsBufferDestroyTest {

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

    /**
     * 1. Translated from all_usages.test()
     * Tests destroying buffers of every usage type.
     */
    @SmallTest
    @Test
    fun testDestroyAllUsages() {
        val usages = listOf(
            BufferUsage.MapRead, BufferUsage.MapWrite, BufferUsage.CopySrc, 
            BufferUsage.CopyDst, BufferUsage.Index, BufferUsage.Vertex, 
            BufferUsage.Uniform, BufferUsage.Storage, BufferUsage.Indirect, 
            BufferUsage.QueryResolve
        )

        for (usage in usages) {
            val buf = device.createBuffer(GPUBufferDescriptor(size = 4, usage = usage, label = "cts"))
            buf.destroy()
        }
    }

    /**
     * 2. Translated from twice.test()
     * Test that destroying a buffer more than once is allowed.
     */
    @SmallTest
    @Test
    fun testDestroyTwice() {
        val cases = listOf(
            Triple(false, BufferUsage.CopySrc, 4L),
            Triple(false, BufferUsage.MapWrite or BufferUsage.CopySrc, 4L),
            Triple(false, BufferUsage.CopyDst or BufferUsage.MapRead, 4L),
            Triple(true, BufferUsage.CopySrc, 4L) // mappedAtCreation
        )

        for (case in cases) {
            val mappedAtCreation = case.first
            val usage = case.second
            val size = case.third

            val buf = device.createBuffer(GPUBufferDescriptor(
                size = size, 
                usage = usage, 
                mappedAtCreation = mappedAtCreation, 
                label = "cts"
            ))

            buf.destroy()
            buf.destroy() // Allowed
        }
    }

    /**
     * 3. Translated from while_mapped.test()
     * Test destroying buffers while mapped or after being unmapped.
     */
    @MediumTest
    @Test
    fun testDestroyWhileMapped() = runBlocking {
        val cases = listOf(
            // mappedAtCreation=true, unmapBeforeDestroy=false
            MapTestParams(mappedAtCreation = true, unmapBeforeDestroy = false, usage = BufferUsage.CopySrc),
            MapTestParams(mappedAtCreation = true, unmapBeforeDestroy = true, usage = BufferUsage.CopySrc),
            
            // Map Mode setups
            MapTestParams(mappedAtCreation = false, unmapBeforeDestroy = false, usage = BufferUsage.MapWrite or BufferUsage.CopySrc, mapMode = MapMode.Write),
            MapTestParams(mappedAtCreation = false, unmapBeforeDestroy = true, usage = BufferUsage.MapWrite or BufferUsage.CopySrc, mapMode = MapMode.Write),
            
            MapTestParams(mappedAtCreation = false, unmapBeforeDestroy = false, usage = BufferUsage.CopyDst or BufferUsage.MapRead, mapMode = MapMode.Read),
            MapTestParams(mappedAtCreation = false, unmapBeforeDestroy = true, usage = BufferUsage.CopyDst or BufferUsage.MapRead, mapMode = MapMode.Read),
            
            // Colliding mappedAtCreation + mapMode mapping shifts
            MapTestParams(mappedAtCreation = true, unmapBeforeDestroy = false, usage = BufferUsage.MapWrite or BufferUsage.CopySrc, mapMode = MapMode.Write),
            MapTestParams(mappedAtCreation = true, unmapBeforeDestroy = false, usage = BufferUsage.CopyDst or BufferUsage.MapRead, mapMode = MapMode.Read)
        )

        for (case in cases) {
            val buf = device.createBuffer(GPUBufferDescriptor(
                size = 4,
                usage = case.usage,
                mappedAtCreation = case.mappedAtCreation,
                label = "cts"
            ))

            if (case.mapMode != null) {
                if (case.mappedAtCreation) {
                    buf.unmap() // Unmap original mappedAtCreation to map again
                }
                buf.mapAndAwait(case.mapMode, 0, 4)
            }
            if (case.unmapBeforeDestroy) {
                buf.unmap()
            }

            buf.destroy()
        }
    }

    private data class MapTestParams(
        val mappedAtCreation: Boolean,
        val unmapBeforeDestroy: Boolean,
        val usage: Int,
        val mapMode: Int? = null
    )
}
