package androidx.webgpu

import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertThrows
import org.junit.Test
import org.junit.After
import org.junit.Before
import org.junit.Ignore
import org.junit.Rule

class WebGpuCtsBufferCreateTest {
    private lateinit var device: GPUDevice
    private lateinit var webGpu: WebGpu

    @get:Rule
    val apiSkipRule = ApiLevelSkipRule()

    @Before
    fun setup() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun teardown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    /**
     * 1. Translated from size.test()
     * Test buffer size alignment is validated to be a multiple of 4 if mappedAtCreation is true.
     */
    @SmallTest
    @Ignore("Native process crash (SIGSEGV/abort). Needs investigation.")
    @Test
    fun testBufferSizeAlignment() {
        val mappedAtCreationOptions = listOf(false, true)
        val sizes = listOf(0L, 2L, 4L, 6L, 8L) // 4 is alignment

        for (mappedAtCreation in mappedAtCreationOptions) {
            for (size in sizes) {
                val isValid = !mappedAtCreation || (size % 4L == 0L)
                val usage = BufferUsage.CopySrc

                if (isValid) {
                    device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, mappedAtCreation = mappedAtCreation, label = "cts")).destroy()
                } else {
                    assertThrows(Exception::class.java) {
                        device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, mappedAtCreation = mappedAtCreation, label = "cts"))
                    }
                }
            }
        }
    }

    /**
     * 2. Translated from limit.test()
     * Test buffer size is validated against maxBufferSize.
     */
    @MediumTest
    @Test
    fun testBufferLimitAgainstMaxBufferSize() {
        val sizeAdditions = listOf(-1, 0, 1)
        val maxBufferSize = device.getLimits().maxBufferSize

        for (addition in sizeAdditions) {
            val size = maxBufferSize + addition
            if (size <= 0) continue // Skip invalid sizes
            val isValid = size <= maxBufferSize
            val usage = BufferUsage.CopySrc

            if (isValid) {
                device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts")).destroy()
            } else {
                assertThrows(Exception::class.java) {
                    device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts"))
                }
            }
        }
    }

    /**
     * 3. Translated from usage.test()
     * Test combinations of two usage flags.
     */
    @MediumTest
    @Ignore("Native process crash (SIGSEGV/abort). Needs investigation.")
    @Test
    fun testBufferUsageCombinations() {
        val usages = listOf(
            BufferUsage.MapRead, BufferUsage.MapWrite, BufferUsage.CopySrc,
            BufferUsage.CopyDst, BufferUsage.Index, BufferUsage.Vertex,
            BufferUsage.Uniform, BufferUsage.Storage, BufferUsage.Indirect,
            BufferUsage.QueryResolve
        )
        val mappedAtCreationOptions = listOf(false, true)

        for (mappedAtCreation in mappedAtCreationOptions) {
            for (usage1 in usages) {
                for (usage2 in usages) {
                    val usage = usage1 or usage2

                    // Validation rule shorthand from CTS
                    val isValid = usage != 0 &&
                        ((usage and BufferUsage.MapRead) == 0 ||
                         (usage and (BufferUsage.CopyDst or BufferUsage.MapRead).inv()) == 0) &&
                        ((usage and BufferUsage.MapWrite) == 0 ||
                         (usage and (BufferUsage.CopySrc or BufferUsage.MapWrite).inv()) == 0)

                    if (isValid) {
                        device.createBuffer(GPUBufferDescriptor(size = 8, usage = usage, mappedAtCreation = mappedAtCreation, label = "cts")).destroy()
                    } else {
                        assertThrows(Exception::class.java) {
                            device.createBuffer(GPUBufferDescriptor(size = 8, usage = usage, mappedAtCreation = mappedAtCreation, label = "cts"))
                        }
                    }
                }
            }
        }
    }

    /**
     * 4. Translated from createBuffer_invalid_and_oom.test()
     * Verifies invalid flag creations are hid behind validation errors.
     */
    @SmallTest
    @Ignore("Native process crash (SIGSEGV/abort). Needs investigation.")
    @Test
    fun testCreateBufferInvalidAndOom() {
        val testCases = listOf(
            Triple(true, BufferUsage.Uniform, 16L),
            Triple(true, BufferUsage.Storage, 16L),
            // Invalid because Uniform not allowed with maps
            Triple(false, BufferUsage.MapWrite or BufferUsage.Uniform, 16L),
            Triple(false, BufferUsage.MapRead or BufferUsage.Uniform, 16L)
        )

        for (case in testCases) {
            val isValid = case.first
            val usage = case.second
            val size = case.third

            if (isValid) {
                device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts")).destroy()
            } else {
                assertThrows(Exception::class.java) {
                    device.createBuffer(GPUBufferDescriptor(size = size, usage = usage, label = "cts"))
                }
            }
        }
    }
}
