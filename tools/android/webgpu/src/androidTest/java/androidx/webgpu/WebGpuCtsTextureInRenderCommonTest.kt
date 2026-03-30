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
class WebGpuCtsTextureInRenderCommonTest {

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

    private fun getColorAttachment(texture: GPUTexture, baseLayer: Int, baseLevel: Int): GPURenderPassColorAttachment {
        val view = texture.createView(
            GPUTextureViewDescriptor(
                baseArrayLayer = baseLayer,
                baseMipLevel = baseLevel,
                arrayLayerCount = 1,
                mipLevelCount = 1
            )
        )
        return GPURenderPassColorAttachment(
            clearValue = GPUColor(0.0, 0.0, 0.0, 0.0),
            view = view,
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store
        )
    }

    @Test
    fun testColorAttachments() = runBlocking {
        val values = intArrayOf(0, 1)

        for (layer0 in values) {
            for (level0 in values) {
                for (layer1 in values) {
                    for (level1 in values) {
                        device.pushErrorScope(ErrorFilter.Validation)

                        // Mip count 2, array layer count 2
                        val texture = device.createTexture(
                            GPUTextureDescriptor(
                                usage = TextureUsage.RenderAttachment,
                                size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 2),
                                mipLevelCount = 2,
                                format = TextureFormat.RGBA8Unorm
                            )
                        )

                        val encoder = device.createCommandEncoder()

                        val attachment0 = getColorAttachment(texture, layer0, level0)
                        val attachment1 = getColorAttachment(texture, layer1, level1)

                        val passDescriptor = GPURenderPassDescriptor(
                            colorAttachments = arrayOf(attachment0, attachment1)
                        )

                        val pass = encoder.beginRenderPass(passDescriptor)
                        pass.end()

                        val error = device.popErrorScope()

                        val success = (layer0 != layer1) || (level0 != level1)

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

    @Test
    fun testColorAttachmentAndBindGroup() = runBlocking {
        val attachments = arrayOf(Pair(0,0), Pair(0,1), Pair(1,0), Pair(1,1))
        
        for (a0 in attachments) {
            for (a1 in attachments) {
                device.pushErrorScope(ErrorFilter.Validation)

                // Mip count 2, array layer count 2
                val texture = device.createTexture(
                    GPUTextureDescriptor(
                        usage = TextureUsage.RenderAttachment or TextureUsage.TextureBinding,
                        size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 2),
                        mipLevelCount = 2,
                        format = TextureFormat.RGBA8Unorm
                    )
                )

                val encoder = device.createCommandEncoder()

                val renderAttachment = getColorAttachment(texture, a0.first, a0.second)

                val bindGroupView = texture.createView(
                    GPUTextureViewDescriptor(
                        baseArrayLayer = a1.first,
                        baseMipLevel = a1.second,
                        arrayLayerCount = 1,
                        mipLevelCount = 1
                    )
                )

                val bglDef = GPUBindGroupLayoutDescriptor(entries = arrayOf(
                    GPUBindGroupLayoutEntry(binding = 0, visibility = ShaderStage.Fragment, texture = GPUTextureBindingLayout(sampleType = TextureSampleType.Float))
                ))

                val bgl = device.createBindGroupLayout(bglDef)
                val bindGroup = device.createBindGroup(GPUBindGroupDescriptor(layout = bgl, entries = arrayOf(
                    GPUBindGroupEntry(binding = 0, textureView = bindGroupView)
                )))

                val passDescriptor = GPURenderPassDescriptor(
                    colorAttachments = arrayOf(renderAttachment)
                )

                val pass = encoder.beginRenderPass(passDescriptor)
                pass.setBindGroup(0, bindGroup)
                pass.end()

                val error = device.popErrorScope()

                // Render attachment is considered a 'write' by WebGPU spec, texture binding is a 'read'.
                // If they overlap to the exact same subresource, it's a conflict!
                val success = (a0.first != a1.first) || (a0.second != a1.second)

                if (success) {
                    assertEquals(ErrorType.NoError, error ?: ErrorType.NoError)
                } else {
                    assertEquals(ErrorType.Validation, error)
                }
            }
        }
    }
}
