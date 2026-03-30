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
class WebGpuCtsTextureInPassEncoderTest {

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

    enum class TestTextureUsage {
        Sampled,
        StorageRead,
        StorageWrite,
        StorageReadWrite,
        RenderAttachment
    }

    private fun getTextureUsageRules(usage: TestTextureUsage): Int {
        return when (usage) {
            TestTextureUsage.Sampled -> TextureUsage.TextureBinding
            TestTextureUsage.StorageRead, TestTextureUsage.StorageWrite, TestTextureUsage.StorageReadWrite -> TextureUsage.StorageBinding
            TestTextureUsage.RenderAttachment -> TextureUsage.RenderAttachment
        }
    }

    @Test
    fun testCombinationForColor() = runBlocking {

        
        val isComputeOptions = arrayOf(true, false)
        val usageOptions = arrayOf(
            TestTextureUsage.Sampled,
            TestTextureUsage.StorageRead,
            TestTextureUsage.StorageWrite,
            TestTextureUsage.StorageReadWrite,
            TestTextureUsage.RenderAttachment
        )

        for (isCompute in isComputeOptions) {
            for (u0 in usageOptions) {
                for (u1 in usageOptions) {
                    // RenderAttachment is not supported in Compute Passes.
                    if (isCompute && (u0 == TestTextureUsage.RenderAttachment || u1 == TestTextureUsage.RenderAttachment)) {
                        continue
                    }

                    device.pushErrorScope(ErrorFilter.Validation)

                    val texture = device.createTexture(
                        GPUTextureDescriptor(
                            usage = getTextureUsageRules(u0) or getTextureUsageRules(u1),
                            size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                            format = TextureFormat.RGBA8Unorm
                        )
                    )

                    val view0 = texture.createView()
                    val view1 = texture.createView() // Same texture, same subresource

                    val makeBg = { v: GPUTextureView, u: TestTextureUsage ->
                        val bglDef = GPUBindGroupLayoutDescriptor(entries = arrayOf(
                            when(u) {
                                TestTextureUsage.Sampled -> GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment or ShaderStage.Compute, texture = GPUTextureBindingLayout(sampleType = TextureSampleType.Float))
                                TestTextureUsage.StorageRead -> GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment or ShaderStage.Compute, storageTexture = GPUStorageTextureBindingLayout(format = TextureFormat.RGBA8Unorm, access = StorageTextureAccess.ReadOnly))
                                TestTextureUsage.StorageWrite -> GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment or ShaderStage.Compute, storageTexture = GPUStorageTextureBindingLayout(format = TextureFormat.RGBA8Unorm, access = StorageTextureAccess.WriteOnly))
                                TestTextureUsage.StorageReadWrite -> GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment or ShaderStage.Compute, storageTexture = GPUStorageTextureBindingLayout(format = TextureFormat.RGBA8Unorm, access = StorageTextureAccess.ReadWrite))
                                else -> GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment or ShaderStage.Compute, texture = GPUTextureBindingLayout(sampleType = TextureSampleType.Float))
                            }
                        ))

                        val bgl = device.createBindGroupLayout(bglDef)
                        device.createBindGroup(GPUBindGroupDescriptor(layout = bgl, entries = arrayOf(
                            GPUBindGroupEntry(binding = 0, textureView = v)
                        )))
                    }

                    val encoder = device.createCommandEncoder()
                    
                    val colorTexture = device.createTexture(
                        GPUTextureDescriptor(
                            usage = TextureUsage.RenderAttachment,
                            size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                            format = TextureFormat.RGBA8Unorm
                        )
                    )

                    if (isCompute) {
                        val pass = encoder.beginComputePass()
                        pass.setBindGroup(0, makeBg(view0, u0))
                        pass.setBindGroup(1, makeBg(view1, u1))
                        pass.end()
                    } else {
                        val renderPassDesc = GPURenderPassDescriptor(colorAttachments = arrayOf(
                            GPURenderPassColorAttachment(clearValue = GPUColor(0.0, 0.0, 0.0, 0.0), view = colorTexture.createView(), loadOp = LoadOp.Clear, storeOp = StoreOp.Store)
                        ))
                        val pass = encoder.beginRenderPass(renderPassDesc)
                        
                        if (u0 != TestTextureUsage.RenderAttachment && u1 != TestTextureUsage.RenderAttachment) {
                            pass.setBindGroup(0, makeBg(view0, u0))
                            pass.setBindGroup(1, makeBg(view1, u1))
                        }
                        pass.end()
                    }

                    encoder.finish()
                    val error = device.popErrorScope()

                    val isReadOnly = { u: TestTextureUsage -> u == TestTextureUsage.Sampled || u == TestTextureUsage.StorageRead }
                    val success = isReadOnly(u0) && isReadOnly(u1)

                    if (success) {
                        assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
                    } else {
                        assertEquals(ErrorType.Validation, error)
                    }
                }
            }
        }
    }
}
