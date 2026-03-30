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
class WebGpuCtsTextureFloat32FilterableTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = runBlocking {
        // Create WebGPU without explicitly requesting "float32-filterable" 
        webGpu = createWebGpu()
        device = webGpu.device
    }

    /**
     * 1. Translated from create_bind_group
     * Test that it is valid to bind a float32 texture format to a 'float' sampled texture iff
     * float32-filterable is enabled. Here we test the default behavior (disabled).
     */
    @Test
    fun testCreateBindGroup(): Unit = runBlocking {
        val formats = listOf(
            TextureFormat.R32Float,
            TextureFormat.RG32Float,
            TextureFormat.RGBA32Float
        )

        for (format in formats) {
            // Case A: sampleType = Float (should fail without the feature)
            checkBindGroupCreation(format, TextureSampleType.Float, expectError = true)

            // Case B: sampleType = UnfilterableFloat (should succeed without the feature)
            checkBindGroupCreation(format, TextureSampleType.UnfilterableFloat, expectError = false)
        }
    }

    private suspend fun checkBindGroupCreation(
        format: Int, 
        sampleType: Int, 
        expectError: Boolean
    ) {
        val entryLayout = GPUBindGroupLayoutEntry(
            binding = 0,
            visibility = ShaderStage.Fragment,
            texture = GPUTextureBindingLayout(sampleType = sampleType)
        )

        val bindGroupLayoutDesc = GPUBindGroupLayoutDescriptor(
            entries = arrayOf(entryLayout)
        )

        val layout = device.createBindGroupLayout(bindGroupLayoutDesc)

        val textureDesc = GPUTextureDescriptor.Builder(
            size = GPUExtent3D(4, 4, 1),
            usage = TextureUsage.TextureBinding
        )
        .setFormat(format)
        .build()

        val texture = device.createTexture(textureDesc)

        val bindGroupEntry = GPUBindGroupEntry(
            binding = 0,
            textureView = texture.createView()
        )

        val bindGroupDesc = GPUBindGroupDescriptor(
            layout = layout,
            entries = arrayOf(bindGroupEntry)
        )

        device.pushErrorScope(ErrorFilter.Validation)
        val bindGroup = device.createBindGroup(bindGroupDesc)

        if (expectError) {
            assertThrows(ValidationException::class.java) {
                runBlocking { device.popErrorScope() }
            }
        } else {
            runBlocking { device.popErrorScope() }
        }

        texture.destroy()
    }
}
