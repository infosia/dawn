package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class WebGpuCtsTextureBgra8unormStorageTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = runBlocking {
        // Without explicitly requesting the "bgra8unorm-storage" feature,
        // it should not be enabled by default.
        webGpu = createWebGpu()
        device = webGpu.device
    }

    /**
     * 1. Translated from create_texture
     * Test that it is validation error to create bgra8unorm texture with STORAGE usage 
     * when the feature bgra8unorm-storage is not requested globally.
     */
    @Test
    fun testCreateTextureWithoutFeature(): Unit = runBlocking {
        val descriptor = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(1, 1, 1),
            usage = TextureUsage.StorageBinding
        )
        .setFormat(TextureFormat.BGRA8Unorm)
        .build()

        device.pushErrorScope(ErrorFilter.Validation)
        val t = device.createTexture(descriptor)

        assertThrows(ValidationException::class.java) {
            runBlocking { device.popErrorScope() }
        }
    }

    /**
     * 2. Translated from create_bind_group_layout
     * Test that it is invalid to create GPUBindGroupLayout that uses bgra8unorm as storage texture format
     * when the feature bgra8unorm-storage is disabled.
     */
    @Test
    fun testCreateBindGroupLayoutWithoutFeature(): Unit = runBlocking {
        val entry = GPUBindGroupLayoutEntry(
            binding = 0,
            visibility = ShaderStage.Compute,
            storageTexture = GPUStorageTextureBindingLayout(
                format = TextureFormat.BGRA8Unorm
            )
        )

        val bindGroupLayoutDescriptor = GPUBindGroupLayoutDescriptor(
            entries = arrayOf(entry)
        )

        device.pushErrorScope(ErrorFilter.Validation)
        val l = device.createBindGroupLayout(bindGroupLayoutDescriptor)

        assertThrows(ValidationException::class.java) {
            runBlocking { device.popErrorScope() }
        }
    }
}
