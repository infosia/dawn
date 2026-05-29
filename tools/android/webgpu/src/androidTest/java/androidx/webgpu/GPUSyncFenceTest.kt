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
import org.junit.After
import org.junit.Assert.assertNotNull
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

    private fun createHardwareBuffer(
        width: Int,
        height: Int,
        format: Int = HardwareBuffer.RGBA_8888,
        usage: Long = HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE
    ): HardwareBuffer {
        return HardwareBuffer.create(width, height, format, 1, usage)
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 30)
    fun testSyncFence_lifecycleAndAwaiting() {
        runBlocking {
            val rgbBuffer = createHardwareBuffer(64, 64)

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.TextureBinding
            )

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()

            if (fence != null) {
                // Await signaling of the sync fence
                val signaled = fence.await(5000)
                assertTrue(signaled)

                // Re-access passing the returned sync fence to beginAccess
                wrapper.beginAccess(arrayOf(fence))
                val nextFence = wrapper.endAccess()
                nextFence?.close()
                fence.close()
            }

            wrapper.close()
            rgbBuffer.close()
        }
    }

    @Test
    @MediumTest
    @ApiRequirement(minApi = 33)
    fun testSyncFence_fromPlatformSyncFence() {
        runBlocking {
            Assume.assumeTrue(Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)

            val rgbBuffer = createHardwareBuffer(64, 64)

            val wrapper = GPUAndroidHardwareBufferUtil.createTexture(
                device,
                rgbBuffer,
                usage = TextureUsage.TextureBinding
            )

            wrapper.beginAccess(null)
            val fence = wrapper.endAccess()

            if (fence != null) {
                // Convert to platform SyncFence using extension method
                val platformSyncFence = fence.toSyncFence()
                assertNotNull(platformSyncFence)

                // Await signaling of the platform SyncFence
                val signaled = platformSyncFence!!.await(java.time.Duration.ofMillis(5000))
                assertTrue(signaled)

                platformSyncFence.close()
                fence.close()
            }

            wrapper.close()
            rgbBuffer.close()
        }
    }
}
