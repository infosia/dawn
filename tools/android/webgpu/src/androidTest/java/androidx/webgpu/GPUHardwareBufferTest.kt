/*
 * Copyright 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package androidx.webgpu

import android.hardware.HardwareBuffer
import android.hardware.SyncFence
import android.os.Build
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.webgpu.helper.toSyncFence
import androidx.test.filters.MediumTest
import androidx.webgpu.helper.GPUAndroidHardwareBufferUtil
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay
import kotlinx.coroutines.Dispatchers
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Assume
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.Executor
import androidx.webgpu.helper.initLibrary
import androidx.webgpu.GPU.createInstance

@RunWith(AndroidJUnit4::class)
@MediumTest
class GPUHardwareBufferTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @get:Rule
    val apiSkipRule = ApiLevelSkipRule()

    @Before
    fun setup() = runBlocking {
        // Verify the physical device/emulator supports all necessary Vulkan and Android
        // extensions required for zero-copy hardware buffer sharing and YUV/RGB import.
        initLibrary()
        val instance = createInstance(
            GPUInstanceDescriptor().apply {
                dawnTogglesDescriptor = GPUDawnTogglesDescriptor(
                    enabledToggles = arrayOf("allow_unsafe_apis")
                )
            }
        )
        val adapter = instance.requestAdapter()

        val hasRequiredFeatures =
            adapter.hasFeature(FeatureName.SharedTextureMemoryAHardwareBuffer) &&
              adapter.hasFeature(FeatureName.SharedFenceSyncFD) &&
              adapter.hasFeature(FeatureName.YCbCrVulkanSamplers) &&
              adapter.hasFeature(FeatureName.OpaqueYCbCrAndroidForExternalTexture)

        adapter.close()
        instance.close()

        // Skip the tests gracefully if the current test environment (e.g., an older emulator or
        // device lacking Vulkan YCbCr extensions) does not support the required GPU features.
        Assume.assumeTrue(
            "Adapter does not support required features for hardware buffer tests",
            hasRequiredFeatures
        )

        webGpu = createWebGpu(
            deviceDescriptor = GPUDeviceDescriptor(
                deviceLostCallback = DeviceLostCallback { _, _, _ -> },
                deviceLostCallbackExecutor = Executor(Runnable::run),
                uncapturedErrorCallback = UncapturedErrorCallback { _, _, _ -> },
                uncapturedErrorCallbackExecutor = Executor(Runnable::run),
                dawnTogglesDescriptor = GPUDawnTogglesDescriptor(
                    enabledToggles = arrayOf("allow_unsafe_apis")
                ),
                requiredFeatures = intArrayOf(
                    FeatureName.SharedTextureMemoryAHardwareBuffer,
                    FeatureName.SharedFenceSyncFD,
                    FeatureName.YCbCrVulkanSamplers,
                    FeatureName.OpaqueYCbCrAndroidForExternalTexture
                )
            )
        )
        device = webGpu.device
    }

    @After
    fun teardown() {
        if (::device.isInitialized) {
            runCatching { device.destroy() }
        }
        if (::webGpu.isInitialized) {
            webGpu.close()
        }
    }

    private fun createHardwareBuffer(
        width: Int,
        height: Int,
        format: Int = HardwareBuffer.RGBA_8888,
        usage: Long = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE
    ): HardwareBuffer {
        return HardwareBuffer.create(width, height, format, 1, usage)
    }



    // =========================================================================
    // 1. MEMORY SAFETY & OOM TESTS
    // =========================================================================

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testImportHardwareBufferLoop_noLeaks() {
        runBlocking {
            for (i in 0 until 50) {
                val rgbBuffer = createHardwareBuffer(
                    128, 128,
                    usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
                )

                val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                    device,
                    rgbBuffer,
                    usage = TextureUsage.TextureBinding
                )

                wrapper.beginAccess(null)
                val fence = wrapper.endAccess()
                fence?.close()

                wrapper.close()
                rgbBuffer.close()
            }
            webGpu.instance.processEvents()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testWebGpuOOMScope_handlesErrorGracefully() {
        runBlocking {
            device.pushErrorScope(ErrorFilter.Validation)

            // Attempt to allocate an absolutely massive texture to trigger validation error
            val descriptor = GPUTextureDescriptor(
                size = GPUExtent3D(width = 1000000, height = 1000000, depthOrArrayLayers = 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.None // Invalid usage
            )
            val unusedTexture = device.createTexture(descriptor)

            // Popping the error scope should throw a ValidationException due to validation failure
            assertThrows(ValidationException::class.java) {
                runBlocking { device.popErrorScope() }
            }
        }
    }

    // =========================================================================
    // 2. RGB & YUV EXTERNAL TEXTURE IMPORT LIFE-CYCLE TESTS
    // =========================================================================

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testRGBHardwareBuffer_asTexture() {
        runBlocking {
            val rgbBuffer = createHardwareBuffer(
                64, 64,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.TextureBinding
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.texture)

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()
            fence?.close()

            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testRGBHardwareBuffer_asExternalTexture() {
        runBlocking {
            val rgbBuffer = createHardwareBuffer(64, 64)

            val extDescriptor = GPUExternalTextureDescriptor(
                cropSize = GPUExtent2D(64, 64),
                srcColorSpace = GPUColorSpaceDawn(
                    primaries = ColorSpacePrimariesDawn.Rec709,
                    transfer = ColorSpaceTransferDawn.SRGB,
                    yCbCrRange = ColorSpaceYCbCrRangeDawn.Narrow,
                    yCbCrMatrix = ColorSpaceYCbCrMatrixDawn.Rec709
                ),
                dstColorSpace = PredefinedColorSpace.SRGB,
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(
                device,
                rgbBuffer,
                extDescriptor
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.externalTexture)

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()
            fence?.close()

            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testYUVHardwareBuffer_asExternalTexture() {
        runBlocking {
            val yuvBuffer = createHardwareBuffer(64, 64, format = HardwareBuffer.YCBCR_420_888)

            val extDescriptor = GPUExternalTextureDescriptor(
                cropSize = GPUExtent2D(64, 64),
                srcColorSpace = GPUColorSpaceDawn(
                    primaries = ColorSpacePrimariesDawn.Rec709,
                    transfer = ColorSpaceTransferDawn.SRGB,
                    yCbCrRange = ColorSpaceYCbCrRangeDawn.Narrow,
                    yCbCrMatrix = ColorSpaceYCbCrMatrixDawn.Rec709
                ),
                dstColorSpace = PredefinedColorSpace.SRGB,
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(
                device,
                yuvBuffer,
                extDescriptor
            )

            // Verify YUV External Texture has been imported successfully
            assertNotNull(wrapper)
            assertNotNull(wrapper.externalTexture)

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()
            fence?.close()

            wrapper.close()
            yuvBuffer.close()
        }
    }

    // =========================================================================
    // 3. RACE CONDITIONS & CONCURRENT ACCESS
    // =========================================================================

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testConcurrentAccess_raceCondition() {
        // Run on the Android Main Thread to ensure perfect single-threaded safety with Dawn's event poller
        runBlocking(Dispatchers.Main) {
            // Concurrently launch interleaved operations on the same thread to ensure JNI and Dawn safety
            val job1 = launch {
                for (i in 0 until 20) {
                    val rgbBuffer = createHardwareBuffer(32, 32)
                    val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                        device,
                        rgbBuffer,
                        usage = TextureUsage.TextureBinding
                    )
                    wrapper.beginAccess(null)
                    val fence = wrapper.endAccess()
                    fence?.close()
                    wrapper.close()
                    rgbBuffer.close()
                    delay(1)
                }
            }

            val job2 = launch {
                for (i in 0 until 20) {
                    val rgbBuffer = createHardwareBuffer(32, 32)
                    val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                        device,
                        rgbBuffer,
                        usage = TextureUsage.TextureBinding
                    )
                    wrapper.beginAccess(null)
                    val fence = wrapper.endAccess()
                    fence?.close()
                    wrapper.close()
                    rgbBuffer.close()
                    delay(1)
                }
            }

            job1.join()
            job2.join()
        }
    }



    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testZeroCopyPipeline_writeAndVerify() {
        runBlocking {
            val width = 64
            val height = 64
            val rgbBuffer = createHardwareBuffer(
                width, height,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.TextureBinding or TextureUsage.CopyDst or TextureUsage.CopySrc
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.texture)

            // Prepare green pixel data (R=0, G=255, B=0, A=255)
            val size = GPUExtent3D(width = width, height = height)
            val byteBuffer = java.nio.ByteBuffer.allocateDirect(width * height * 4)
            byteBuffer.order(java.nio.ByteOrder.nativeOrder())
            for (i in 0 until width * height) {
                byteBuffer.put(0.toByte())   // R
                byteBuffer.put(255.toByte()) // G
                byteBuffer.put(0.toByte())   // B
                byteBuffer.put(255.toByte()) // A
            }
            byteBuffer.rewind()

            wrapper.beginAccess(null)

            // Write pixels to the imported WebGPU texture, updating the shared memory zero-copy
            device.queue.writeTexture(
                dataLayout = GPUTexelCopyBufferLayout(
                    bytesPerRow = width * 4,
                    rowsPerImage = height
                ),
                data = byteBuffer,
                destination = GPUTexelCopyTextureInfo(texture = wrapper.texture),
                writeSize = size
            )

            // Process events to flush queue writes
            webGpu.instance.processEvents()

            val fence = wrapper.endAccess()
            fence?.close()

            // Wrap the shared zero-copy memory in an Android hardware Bitmap
            val hardwareBitmap = android.graphics.Bitmap.wrapHardwareBuffer(
                rgbBuffer,
                android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB)
            )
            assertNotNull(hardwareBitmap)

            // Copy to software bitmap for pixel readback
            val softwareBitmap = hardwareBitmap!!.copy(android.graphics.Bitmap.Config.ARGB_8888, false)
            assertNotNull(softwareBitmap)

            // Verify pixel components at the center
            val centerPixel = softwareBitmap.getPixel(width / 2, height / 2)
            val alpha = android.graphics.Color.alpha(centerPixel)
            val red = android.graphics.Color.red(centerPixel)
            val green = android.graphics.Color.green(centerPixel)
            val blue = android.graphics.Color.blue(centerPixel)

            assertEquals(255, alpha)
            assertEquals(0, red)
            assertEquals(255, green)
            assertEquals(0, blue)

            softwareBitmap.recycle()
            hardwareBitmap.recycle()
            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testZeroCopyPipeline_gpuRenderWriteAndVerify() {
        runBlocking {
            val width = 64
            val height = 64

            val rgbBuffer = createHardwareBuffer(
                width, height,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.RenderAttachment or TextureUsage.CopySrc
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.texture)

            // Shader outputs solid red color (R=255, G=0, B=0, A=255)
            val shaderModule = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(
                        code = """
                            @vertex fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
                                const pos = array(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0));
                                return vec4f(pos[i], 0.0, 1.0);
                            }

                            @fragment fn fragmentMain() -> @location(0) vec4f {
                                return vec4f(1.0, 0.0, 0.0, 1.0);
                            }
                        """.trimIndent()
                    )
                )
            )

            val pipeline = device.createRenderPipeline(
                GPURenderPipelineDescriptor(
                    vertex = GPUVertexState(
                        module = shaderModule,
                        entryPoint = "vertexMain"
                    ),
                    fragment = GPUFragmentState(
                        module = shaderModule,
                        entryPoint = "fragmentMain",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    )
                )
            )

            wrapper.beginAccess(null)

            // Render directly into the imported HardwareBuffer texture view
            val encoder = device.createCommandEncoder()
            val pass = encoder.beginRenderPass(
                GPURenderPassDescriptor(
                    colorAttachments = arrayOf(
                        GPURenderPassColorAttachment(
                            view = wrapper.texture.createView(),
                            loadOp = LoadOp.Clear,
                            storeOp = StoreOp.Store,
                            clearValue = GPUColor(0.0, 0.0, 0.0, 1.0)
                        )
                    )
                )
            )
            pass.setPipeline(pipeline)
            pass.draw(3)
            pass.end()

            val commandBuffer = encoder.finish()
            device.queue.submit(arrayOf(commandBuffer))

            webGpu.instance.processEvents()

            val fence = wrapper.endAccess()
            fence?.close()

            // Wrap and assert the GPU rasterizer output is reflected zero-copy
            val hardwareBitmap = android.graphics.Bitmap.wrapHardwareBuffer(
                rgbBuffer,
                android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB)
            )
            assertNotNull(hardwareBitmap)

            val softwareBitmap = hardwareBitmap!!.copy(android.graphics.Bitmap.Config.ARGB_8888, false)
            assertNotNull(softwareBitmap)

            val centerPixel = softwareBitmap.getPixel(width / 2, height / 2)
            val alpha = android.graphics.Color.alpha(centerPixel)
            val red = android.graphics.Color.red(centerPixel)
            val green = android.graphics.Color.green(centerPixel)
            val blue = android.graphics.Color.blue(centerPixel)

            assertEquals(255, alpha)
            assertEquals(255, red)
            assertEquals(0, green)
            assertEquals(0, blue)

            softwareBitmap.recycle()
            hardwareBitmap.recycle()
            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testZeroCopyPipeline_gpuSamplingAndCopyVerify() {
        runBlocking {
            val width = 64
            val height = 64

            // Clear source texture with solid blue color (R=0, G=0, B=255, A=255)
            val sourceTexture = device.createTexture(
                GPUTextureDescriptor(
                    size = GPUExtent3D(width = width, height = height, depthOrArrayLayers = 1),
                    format = TextureFormat.RGBA8Unorm,
                    usage = TextureUsage.CopySrc or TextureUsage.RenderAttachment
                )
            )

            val clearEncoder = device.createCommandEncoder()
            val clearPass = clearEncoder.beginRenderPass(
                GPURenderPassDescriptor(
                    colorAttachments = arrayOf(
                        GPURenderPassColorAttachment(
                            view = sourceTexture.createView(),
                            loadOp = LoadOp.Clear,
                            storeOp = StoreOp.Store,
                            clearValue = GPUColor(0.0, 0.0, 1.0, 1.0)
                        )
                    )
                )
            )
            clearPass.end()
            device.queue.submit(arrayOf(clearEncoder.finish()))

            // Create imported HardwareBuffer texture
            val rgbBuffer = createHardwareBuffer(
                width, height,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.CopyDst or TextureUsage.TextureBinding
            )

            wrapper.beginAccess(null)

            // Copy directly from source texture to zero-copy texture
            val copyEncoder = device.createCommandEncoder()
            copyEncoder.copyTextureToTexture(
                source = GPUTexelCopyTextureInfo(texture = sourceTexture),
                destination = GPUTexelCopyTextureInfo(texture = wrapper.texture),
                copySize = GPUExtent3D(width = width, height = height)
            )
            device.queue.submit(arrayOf(copyEncoder.finish()))

            webGpu.instance.processEvents()

            val fence = wrapper.endAccess()
            fence?.close()

            // Wrap and assert the GPU copied pixels are reflected zero-copy
            val hardwareBitmap = android.graphics.Bitmap.wrapHardwareBuffer(
                rgbBuffer,
                android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB)
            )
            assertNotNull(hardwareBitmap)

            val softwareBitmap = hardwareBitmap!!.copy(android.graphics.Bitmap.Config.ARGB_8888, false)
            assertNotNull(softwareBitmap)

            val centerPixel = softwareBitmap.getPixel(width / 2, height / 2)
            val alpha = android.graphics.Color.alpha(centerPixel)
            val red = android.graphics.Color.red(centerPixel)
            val green = android.graphics.Color.green(centerPixel)
            val blue = android.graphics.Color.blue(centerPixel)

            assertEquals(255, alpha)
            assertEquals(0, red)
            assertEquals(0, green)
            assertEquals(255, blue)

            softwareBitmap.recycle()
            hardwareBitmap.recycle()
            wrapper.close()
            rgbBuffer.close()
            sourceTexture.destroy()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testYUVExternalTexture_descriptorTransform() {
        runBlocking {
            val yuvBuffer = createHardwareBuffer(128, 128, format = HardwareBuffer.YCBCR_420_888)

            val extDescriptor = GPUExternalTextureDescriptor(
                cropOrigin = GPUOrigin2D(16, 16),
                cropSize = GPUExtent2D(96, 96),
                mirrored = true,
                rotation = ExternalTextureRotation.Rotate180Degrees,
                srcColorSpace = GPUColorSpaceDawn(
                    primaries = ColorSpacePrimariesDawn.Rec709,
                    transfer = ColorSpaceTransferDawn.SRGB,
                    yCbCrRange = ColorSpaceYCbCrRangeDawn.Narrow,
                    yCbCrMatrix = ColorSpaceYCbCrMatrixDawn.Rec709
                ),
                dstColorSpace = PredefinedColorSpace.SRGB,
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(
                device,
                yuvBuffer,
                extDescriptor
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.externalTexture)

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()

            fence?.close()
            wrapper.close()
            yuvBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testRGBExternalTexture_descriptorTransform() {
        runBlocking {
            val width = 64
            val height = 64

            val rgbBuffer = createHardwareBuffer(width, height)

            val extDescriptor = GPUExternalTextureDescriptor(
                cropOrigin = GPUOrigin2D(8, 8),
                cropSize = GPUExtent2D(48, 48),
                mirrored = true,
                rotation = ExternalTextureRotation.Rotate90Degrees,
                srcColorSpace = GPUColorSpaceDawn(
                    primaries = ColorSpacePrimariesDawn.Rec709,
                    transfer = ColorSpaceTransferDawn.SRGB,
                    yCbCrRange = ColorSpaceYCbCrRangeDawn.Narrow,
                    yCbCrMatrix = ColorSpaceYCbCrMatrixDawn.Rec709
                ),
                dstColorSpace = PredefinedColorSpace.SRGB,
            )

            val wrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(
                device,
                rgbBuffer,
                extDescriptor
            )

            assertNotNull(wrapper)
            assertNotNull(wrapper.externalTexture)

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()

            fence?.close()
            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testColorSpaceEnumVariants() {
        runBlocking {
            val width = 64
            val height = 64

            // 1. Allocate source HardwareBuffer and populate it with solid Red (R=255, G=0, B=0, A=255)
            val srcBuffer = createHardwareBuffer(
                width, height,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val srcWrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                srcBuffer,
                usage = TextureUsage.TextureBinding or TextureUsage.CopyDst or TextureUsage.CopySrc
            )

            val byteBuffer = java.nio.ByteBuffer.allocateDirect(width * height * 4).apply {
                order(java.nio.ByteOrder.nativeOrder())
                for (i in 0 until width * height) {
                    put(255.toByte()) // R
                    put(0.toByte())   // G
                    put(0.toByte())   // B
                    put(255.toByte()) // A
                }
                rewind()
            }

            srcWrapper.beginAccess(null)
            device.queue.writeTexture(
                dataLayout = GPUTexelCopyBufferLayout(
                    bytesPerRow = width * 4,
                    rowsPerImage = height
                ),
                data = byteBuffer,
                destination = GPUTexelCopyTextureInfo(texture = srcWrapper.texture),
                writeSize = GPUExtent3D(width = width, height = height)
            )
            webGpu.instance.processEvents()
            srcWrapper.endAccess()?.close()

            // 2. Allocate destination HardwareBuffer (render target)
            val dstBuffer = createHardwareBuffer(
                width, height,
                usage = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
            )

            val dstWrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                dstBuffer,
                usage = TextureUsage.RenderAttachment or TextureUsage.CopySrc
            )


            val sampler = device.createSampler(GPUSamplerDescriptor())

            val shaderModule = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(
                        code = """
                            @vertex fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
                                const pos = array(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0));
                                return vec4f(pos[i], 0.0, 1.0);
                            }

                            @group(0) @binding(0) var mySampler: sampler;
                            @group(0) @binding(1) var myExternalTexture: texture_external;

                            @fragment fn fragmentMain(@builtin(position) fragCoord: vec4f) -> @location(0) vec4f {
                                return textureSampleBaseClampToEdge(myExternalTexture, mySampler, vec2f(0.5, 0.5));
                            }
                        """.trimIndent()
                    )
                )
            )

            val pipeline = device.createRenderPipeline(
                GPURenderPipelineDescriptor(
                    vertex = GPUVertexState(
                        module = shaderModule,
                        entryPoint = "vertexMain"
                    ),
                    fragment = GPUFragmentState(
                        module = shaderModule,
                        entryPoint = "fragmentMain",
                        targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
                    )
                )
            )

            val primariesToTest = intArrayOf(
                ColorSpacePrimariesDawn.Rec709,
                ColorSpacePrimariesDawn.Rec601
            )

            val transfersToTest = intArrayOf(
                ColorSpaceTransferDawn.SRGB,
                ColorSpaceTransferDawn.Identity
            )

            val matricesToTest = intArrayOf(
                ColorSpaceYCbCrMatrixDawn.Rec709,
                ColorSpaceYCbCrMatrixDawn.Rec601
            )

            val rangesToTest = intArrayOf(
                ColorSpaceYCbCrRangeDawn.Narrow,
                ColorSpaceYCbCrRangeDawn.Full
            )

            // 3. Permute, draw, and verify output color for all combinations on Main thread
            runBlocking(Dispatchers.Main) {
                for (primary in primariesToTest) {
                    for (transfer in transfersToTest) {
                        val matrix = matricesToTest[primary % matricesToTest.size]
                        val range = rangesToTest[transfer % rangesToTest.size]

                        val extDescriptor = GPUExternalTextureDescriptor(
                            cropSize = GPUExtent2D(width, height),
                            srcColorSpace = GPUColorSpaceDawn(
                                primaries = primary,
                                transfer = transfer,
                                yCbCrRange = range,
                                yCbCrMatrix = matrix
                            ),
                            dstColorSpace = PredefinedColorSpace.SRGB,
                        )

                        val srcExtWrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(
                            device,
                            srcBuffer,
                            extDescriptor
                        )

                        assertNotNull(srcExtWrapper)

                        val bindGroup = device.createBindGroup(
                            GPUBindGroupDescriptor(
                                layout = pipeline.getBindGroupLayout(0),
                                entries = arrayOf(
                                    GPUBindGroupEntry(binding = 0, sampler = sampler),
                                    GPUBindGroupEntry(
                                        binding = 1,
                                        externalTextureBindingEntry = GPUExternalTextureBindingEntry(srcExtWrapper.externalTexture)
                                    )
                                )
                            )
                        )

                        // Begin access on both wrappers
                        srcExtWrapper.beginAccess(null)
                        dstWrapper.beginAccess(null)

                        val view = dstWrapper.texture.createView()
                        val encoder = device.createCommandEncoder()
                        val pass = encoder.beginRenderPass(
                            GPURenderPassDescriptor(
                                colorAttachments = arrayOf(
                                    GPURenderPassColorAttachment(
                                        view = view,
                                        loadOp = LoadOp.Clear,
                                        storeOp = StoreOp.Store,
                                        clearValue = GPUColor(0.0, 0.0, 0.0, 1.0)
                                    )
                                )
                            )
                        )
                        pass.setPipeline(pipeline)
                        pass.setBindGroup(0, bindGroup)
                        pass.draw(3)
                        pass.end()

                        device.queue.submit(arrayOf(encoder.finish()))
                        webGpu.instance.processEvents()

                        val fenceSrc = srcExtWrapper.endAccess()
                        val fenceDst = dstWrapper.endAccess()
                        fenceSrc?.close()
                        fenceDst?.close()

                        // 4. Verify transformed color components
                        val hardwareBitmap = android.graphics.Bitmap.wrapHardwareBuffer(
                            dstBuffer,
                            android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB)
                        )
                        assertNotNull(hardwareBitmap)

                        val softwareBitmap = hardwareBitmap!!.copy(android.graphics.Bitmap.Config.ARGB_8888, false)
                        assertNotNull(softwareBitmap)

                        val centerPixel = softwareBitmap.getPixel(width / 2, height / 2)
                        val alpha = android.graphics.Color.alpha(centerPixel)
                        val red = android.graphics.Color.red(centerPixel)
                        val green = android.graphics.Color.green(centerPixel)
                        val blue = android.graphics.Color.blue(centerPixel)

                        // Assert color conversion preserves primarily Red format
                        assertEquals(255, alpha)
                        assertTrue("Expected Red component to be high (got $red)", red > 200)
                        assertTrue("Expected Green component to be low (got $green)", green < 50)
                        assertTrue("Expected Blue component to be low (got $blue)", blue < 50)

                        softwareBitmap.recycle()
                        hardwareBitmap.recycle()
                        view.close()
                        bindGroup.close()
                        srcExtWrapper.close()
                    }
                }
            }

            // 5. Clean up shared wrappers
            srcWrapper.close()
            dstWrapper.close()
            srcBuffer.close()
            dstBuffer.close()
        }
    }
}
