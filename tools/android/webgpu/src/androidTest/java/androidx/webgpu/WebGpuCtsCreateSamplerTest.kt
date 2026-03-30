package androidx.webgpu

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

@RunWith(AndroidJUnit4::class)
@SmallTest

class WebGpuCtsCreateSamplerTest {
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

    private fun doCreateSamplerTest(
        success: Boolean,
        descriptor: GPUSamplerDescriptor
    ) = runBlocking {
        device.pushErrorScope(ErrorFilter.Validation)
        var errorThrown = false
        try {
            device.createSampler(descriptor)
            val error = device.popErrorScope()
            if (error != ErrorType.NoError) {
                errorThrown = true
            }
        } catch (e: Exception) {
            // Uncaptured error callback might throw
            errorThrown = true
            try { device.popErrorScope() } catch (ignored: Exception) {}
        }
        if (success) {
            assert(!errorThrown) { "Expected success but got validation error" }
        } else {
            assert(errorThrown) { "Expected validation error but got success" }
        }
    }

    @Test
    fun testLodMinAndMaxClamp(): Unit = runBlocking {
        val clamps = listOf(-4e-30f, -1f, 0f, 0.5f, 1f, 10f, 4e30f)
        
        for (lodMinClamp in clamps) {
            for (lodMaxClamp in clamps) {
                val descriptor = GPUSamplerDescriptor(
                    lodMinClamp = lodMinClamp,
                    lodMaxClamp = lodMaxClamp
                )
                
                val success = lodMinClamp <= lodMaxClamp && lodMinClamp >= 0f && lodMaxClamp >= 0f
                doCreateSamplerTest(success, descriptor)
            }
        }
    }

    @Test
    fun testMaxAnisotropy(): Unit = runBlocking {
        class TestCase(
            val maxAnisotropy: Short,
            val minFilter: Int,
            val magFilter: Int,
            val mipmapFilter: Int
        )
        
        val maxAnisotropyValues = listOf<Short>(-1, 0, 1, 2, 4, 7, 16, 32, 33, 1024)
        
        val cases = mutableListOf<TestCase>()
        
        for (aniso in maxAnisotropyValues) {
            // First loop: linear filters with varying anisotropy
            cases.add(
                TestCase(
                    maxAnisotropy = aniso,
                    minFilter = FilterMode.Linear,
                    magFilter = FilterMode.Linear,
                    mipmapFilter = MipmapFilterMode.Linear
                )
            )
            // Second loop: varying minFilter with nearest
            cases.add(
                TestCase(
                    maxAnisotropy = aniso,
                    minFilter = FilterMode.Nearest,
                    magFilter = FilterMode.Linear,
                    mipmapFilter = MipmapFilterMode.Linear
                )
            )
            // Third loop: varying magFilter with nearest
            cases.add(
                TestCase(
                    maxAnisotropy = aniso,
                    minFilter = FilterMode.Linear,
                    magFilter = FilterMode.Nearest,
                    mipmapFilter = MipmapFilterMode.Linear
                )
            )
            // Fourth loop: varying mipmapFilter with nearest
            cases.add(
                TestCase(
                    maxAnisotropy = aniso,
                    minFilter = FilterMode.Linear,
                    magFilter = FilterMode.Linear,
                    mipmapFilter = MipmapFilterMode.Nearest
                )
            )
        }
        
        for (case in cases) {
            val descriptor = GPUSamplerDescriptor(
                maxAnisotropy = case.maxAnisotropy,
                minFilter = case.minFilter,
                magFilter = case.magFilter,
                mipmapFilter = case.mipmapFilter
            )
            
            val actualAniso = case.maxAnisotropy.toUShort().toInt()
            // Restore the strict WebGPU CTS constraints: maxAnisotropy > 1 requires all linear filters.
            val success = actualAniso >= 1 && 
                !(actualAniso > 1 && 
                  !(case.minFilter == FilterMode.Linear && 
                    case.magFilter == FilterMode.Linear && 
                    case.mipmapFilter == MipmapFilterMode.Linear))
            try {
                doCreateSamplerTest(success, descriptor)
            } catch (e: AssertionError) {
                throw AssertionError("Failed on case: maxAnisotropy=${case.maxAnisotropy}, min=${case.minFilter}, mag=${case.magFilter}, mipmap=${case.mipmapFilter}. Expected success=$success", e)
            }
        }
    }
}
