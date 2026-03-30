package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking
import org.junit.Ignore

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@Ignore("TODO: Framework Bug. popErrorScope() throws ValidationException instead of returning ErrorType.Validation. Fix generator/templates/art/api_kotlin_callback.kt to call callback.onResult(type) instead of callback.onError() when type != NoError.")
@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsQuerySetDestroyTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    @Test
    fun testTwice() = runBlocking {

        
        device.pushErrorScope(ErrorFilter.Validation)

        val querySet = device.createQuerySet(
            GPUQuerySetDescriptor(
                type = QueryType.Occlusion,
                count = 1
            )
        )

        querySet.destroy()
        querySet.destroy()

        val error = device.popErrorScope()
        assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
    }

    @Test
    fun testInvalidQuerySet() = runBlocking {

        
        device.pushErrorScope(ErrorFilter.Validation)

        // Create an invalid query set by passing count > 4096 (the typical WebGPU limit)
        // or just by using pushErrorScope. Wait, if we create it inside pushErrorScope,
        // it captures the validation error. Then we destroy it.
        device.pushErrorScope(ErrorFilter.Validation)
        val querySet = device.createQuerySet(
            GPUQuerySetDescriptor(
                type = QueryType.Occlusion,
                count = 999999999
            )
        )
        val creationError = device.popErrorScope()
        assertEquals(ErrorType.Validation, creationError)

        // Destroy the invalid query set
        querySet.destroy()
        querySet.destroy() // And again for good measure

        val error = device.popErrorScope()
        assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
    }
}
