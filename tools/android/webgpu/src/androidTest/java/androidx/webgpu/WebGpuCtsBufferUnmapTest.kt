package androidx.webgpu

import androidx.webgpu.helper.createWebGpu
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Validation tests for GPUBuffer.unmap.
 * Translated from third_party/webgpu-cts/src/webgpu/api/validation/buffer/mapping.spec.ts
 */
@RunWith(AndroidJUnit4::class)
class WebGpuCtsBufferUnmapTest {

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
     * 1. Translated from unmap,state,unmapped
     * Test it is valid to call unmap on a buffer that is already unmapped.
     */
    @SmallTest
    @Test
    fun testUnmapStateUnmapped() = runBlocking {
        // 1. Unmapped right after creation
        val buffer1 = createMappableBuffer(MapMode.Read)
        buffer1.unmap() // Valid

        // 2. Unmapped after mapAsync + unmap
        val buffer2 = createMappableBuffer(MapMode.Read)
        buffer2.mapAndAwait(MapMode.Read, 0, 16)
        buffer2.unmap()
        buffer2.unmap() // Valid to call twice

        // 3. Unmapped after mappedAtCreation + unmap
        val buffer3 = device.createBuffer(GPUBufferDescriptor(
            size = 16, 
            usage = BufferUsage.MapRead, 
            mappedAtCreation = true, 
            label = "cts"
        ))
        buffer3.unmap()
        buffer3.unmap() // Valid to call twice
    }

    /**
     * 2. Translated from unmap,state,destroyed
     * Test it is valid to call unmap on a buffer that is destroyed.
     */
    @SmallTest
    @Test
    fun testUnmapStateDestroyed() = runBlocking {
        // 1. Destroyed before unmapped
        val buffer1 = createMappableBuffer(MapMode.Read)
        buffer1.destroy()
        buffer1.unmap()

        // 2. Destroyed after mapAsync
        val buffer2 = createMappableBuffer(MapMode.Read)
        buffer2.mapAndAwait(MapMode.Read, 0, 16)
        buffer2.destroy()
        buffer2.unmap()

        // 3. Destroyed after mappedAtCreation
        val buffer3 = device.createBuffer(GPUBufferDescriptor(
            size = 16, 
            usage = BufferUsage.MapRead, 
            mappedAtCreation = true, 
            label = "cts"
        ))
        buffer3.destroy()
        buffer3.unmap()
    }

    /**
     * 3. Translated from unmap,state,mappedAtCreation
     * Test calling unmap on a buffer mapped at creation.
     */
    @SmallTest
    @Test
    fun testUnmapStateMappedAtCreation() {
        val usages = listOf(
            BufferUsage.MapRead, BufferUsage.MapWrite, BufferUsage.CopySrc, 
            BufferUsage.CopyDst, BufferUsage.Index, BufferUsage.Vertex, 
            BufferUsage.Uniform, BufferUsage.Storage, BufferUsage.Indirect, 
            BufferUsage.QueryResolve
        )

        for (usage in usages) {
            val buffer = device.createBuffer(GPUBufferDescriptor(
                size = 16, 
                usage = usage, 
                mappedAtCreation = true, 
                label = "cts"
            ))

            buffer.unmap()
        }
    }

    /**
     * 4. Translated from unmap,state,mapped
     * Test it is valid to call unmap on a mapped buffer.
     */
    @SmallTest
    @Test
    fun testUnmapStateMapped() = runBlocking {
        val mapModes = listOf(MapMode.Read, MapMode.Write)

        for (mapMode in mapModes) {
            val buffer = createMappableBuffer(mapMode)
            buffer.mapAndAwait(mapMode, 0, 16)
            buffer.unmap()
        }
    }
}
