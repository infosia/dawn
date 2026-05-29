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
import android.os.Build
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.webgpu.GPU.createInstance
import androidx.webgpu.helper.GPUAndroidHardwareBufferUtil
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import androidx.webgpu.helper.initLibrary
import androidx.webgpu.helper.toSyncFence
import java.util.concurrent.Executor
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assume
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@MediumTest
class GPUSyncFenceTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @get:Rule
    val apiSkipRule = ApiLevelSkipRule()

    @Before
    fun setup() = runBlocking {
        // Verify the physical device/emulator supports all necessary Vulkan and Android
        // extensions required for zero-copy hardware buffer sharing and synchronization.
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
              adapter.hasFeature(FeatureName.SharedFenceSyncFD)

        adapter.close()
        instance.close()

        // Skip the tests gracefully if the current test environment
        // does not support the required GPU features.
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
                    FeatureName.SharedFenceSyncFD
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

    private fun setupPipelineAndDraw(wrapper: GPUHardwareBufferTexture): GPURenderPipeline {
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

        drawToTexture(wrapper, pipeline)
        // Process events to flush the queue to Vulkan before extracting the Sync FD
        webGpu.instance.processEvents()

        return pipeline
    }

    private fun drawToTexture(wrapper: GPUHardwareBufferTexture, pipeline: GPURenderPipeline) {
        wrapper.beginAccess(null)
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

        device.queue.submit(arrayOf(encoder.finish()))
    }

    /**
     * Verifies exporting and awaiting a GPUSyncFence.
     */
    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testSyncFence_lifecycleAndAwaiting() = runBlocking {
        val (rgbBuffer, wrapper) = createTestTextureWrapper()

        val pipeline = setupPipelineAndDraw(wrapper)
        val fence = wrapper.endAccess()
        assertNotNull("Sync fence should not be null", fence)

        try {
            // Await signaling of the sync fence
            val signaled = fence!!.await(5000)
            assertTrue(signaled)

            // Re-access the texture.
            drawToTexture(wrapper, pipeline)

            webGpu.instance.processEvents()

            val nextFence = wrapper.endAccess()
            assertNotNull("Sync fence should not be null", nextFence)
            assertTrue(nextFence!!.await(5000))
            nextFence.close()
        } finally {
            wrapper.close()
            rgbBuffer.close()
        }
    }

    /**
     * Verifies importing a GPUSyncFence from an Android SyncFence.
     */
    @Test
    @MediumTest
    @ApiRequirement(minApi = 33)
    fun testSyncFence_fromPlatformSyncFence() = runBlocking {
        Assume.assumeTrue(Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)

        val (rgbBuffer, wrapper) = createTestTextureWrapper()

        setupPipelineAndDraw(wrapper)
        val fence = wrapper.endAccess()
        assertNotNull("Sync fence should not be null", fence)

        try {
            // Convert to platform SyncFence using extension method
            val platformSyncFence = fence!!.toSyncFence()
            assertNotNull(platformSyncFence)

            // Await signaling of the platform SyncFence
            val signaled = platformSyncFence!!.await(java.time.Duration.ofMillis(5000))
            assertTrue(signaled)

            platformSyncFence.close()
        } finally {
            wrapper.close()
            rgbBuffer.close()
        }
    }

    private fun createTestTextureWrapper(): Pair<HardwareBuffer, GPUHardwareBufferTexture> {
        val rgbBuffer = HardwareBuffer.create(
            64, 64, HardwareBuffer.RGBA_8888, 1,
            HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
        )
        val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
            device, rgbBuffer,
            usage = TextureUsage.RenderAttachment or TextureUsage.CopySrc
        )
        return Pair(rgbBuffer, wrapper)
    }
}
