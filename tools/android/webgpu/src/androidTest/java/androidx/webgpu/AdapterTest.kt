package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.initLibrary
import java.util.concurrent.Executor
import junit.framework.TestCase.assertEquals
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class AdapterTest {
  private lateinit var instance: GPUInstance
  private lateinit var coreAdapter: GPUAdapter
  private lateinit var compatAdapter: GPUAdapter

  @Before
  fun setup() = runBlocking {
    initLibrary()
    instance = createInstance()

    //Request the default/Core adapter (Vulkan).
    coreAdapter = instance.requestAdapter(RequestAdapterOptions(featureLevel = FeatureLevel.Core))

    /**
     * Request the Compatibility adapter (GLES/etc.).
     * This adapter is explicitly constrained to the less-capable Compatibility feature set.
     * We explicitly specify BackendType.OpenGLES to force the adapter to use the GLES driver path
     */
    compatAdapter =
      instance.requestAdapter(
        RequestAdapterOptions(
          featureLevel = FeatureLevel.Compatibility,
          backendType = BackendType.OpenGLES,
        )
      )
  }

  @After
  fun teardown() {
    coreAdapter.close()
    compatAdapter.close()
    instance.close()
  }

  /**
   * Helper map for **Core** default limit values as defined by the WebGPU spec.
   * This set of limits is expected when requesting a device from the Core (Vulkan) adapter.
   */
  companion object {
    private val kCoreLimits = mapOf(
      "maxTextureDimension2D" to 8192,
      "maxBindGroups" to 4,
      "minUniformBufferOffsetAlignment" to 256
    )
  }

  @Test
  fun coreAdapterBackendTest() {
    val adapterInfo = coreAdapter.getInfo()
    assertEquals(
      "The backend type should be Vulkan",
      BackendType.Vulkan, adapterInfo.backendType
    )
  }

  @Test
  fun compatAdapterBackendTest() {
    val adapterInfo = compatAdapter.getInfo()
    assertEquals(
      "The backend type should be OpenGLES",
      BackendType.OpenGLES, adapterInfo.backendType
    )
  }

  private suspend fun requestTestDevice(
    adapter: GPUAdapter,
    featureToTest: IntArray = intArrayOf(),
    limits: Limits = Limits(),
  ): GPUDevice {
    val descriptor = DeviceDescriptor(
      requiredFeatures = featureToTest,
      requiredLimits = limits,
      deviceLostCallback = DeviceLostCallback { device, reason, message ->
        throw DeviceLostException(device, reason, message)
      },
      deviceLostCallbackExecutor = Executor(Runnable::run),
      uncapturedErrorCallback = UncapturedErrorCallback { _, type, message ->
        throw RuntimeException("Uncaptured error: $type, $message")
      },
      uncapturedErrorCallbackExecutor = Executor(Runnable::run),
    )
    return adapter.requestDevice(descriptor)
  }

  /**
   * Verifies that requesting a device from the **Core Adapter** with default parameters returns a device
   * with Core features enabled.
   */
  @Test
  fun requestDeviceWithDefaultParametersEnablesCoreFeatures_CoreAdapter() {
    val device = runBlocking { coreAdapter.requestDevice() }
    try {
      val features = device.getFeatures().features
      // In Core mode, the baseline feature set is explicitly listed.
      assertArrayEquals(intArrayOf(FeatureName.CoreFeaturesAndLimits), features)
    } finally {
      device.destroy()
    }
  }

  /**
   * Verifies that requesting a device from the Compatibility Adapter
   * returns a device with an empty features list, as it relies on the GLES backend
   * and does not explicitly advertise the Core feature set.
   */
  @Test
  fun requestDeviceWithDefaultParametersEnablesCoreFeatures_CompatibilityAdapter() {
    val device = runBlocking { compatAdapter.requestDevice() }
    try {
      val features = device.getFeatures().features
      // In Compatibility mode, no distinct core feature flag is expected.
      assertArrayEquals(intArrayOf(), features)
    } finally {
      device.destroy()
    }
  }

  /**
   * Verifies that requesting a device with default parameters returns a device
   * with the correct, specification-defined default limits.
   */
  @Test
  fun requestDeviceWithDefaultParametersReturnsDefaultLimits() {
    val device = runBlocking { coreAdapter.requestDevice() }
    try {
      val limits = device.getLimits()
      assertEquals(
        "maxTextureDimension2D should be the default value",
        kCoreLimits["maxTextureDimension2D"],
        limits.maxTextureDimension2D
      )
      assertEquals(
        "maxBindGroups should be the default value",
        kCoreLimits["maxBindGroups"],
        limits.maxBindGroups
      )
    } finally {
      device.destroy()
    }
  }

  /**
   * Tests that an adapter can only grant a device once. Subsequent requests fail.
   *
   * CTS Test: requestAdapter_stale
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">WebGPU CTS Test</a>
   */
  @Test
  fun requestDeviceFailsAfterSuccessfulRequest() {
    val device = runBlocking { coreAdapter.requestDevice() }
    device.destroy()
    assertThrows(
      "Adapter should be consumed after one device request", WebGpuException::class.java
    ) {
      runBlocking {
        val secondDeviceStatus = runBlocking { coreAdapter.requestDevice() }
      }
    }
  }

  /**
   * Tests requesting a device with a feature that the adapter supports.
   *
   * CTSTest: features,known
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">WebGPU CTS Test</a>
   */
  @Test
  fun requestDeviceWithSupportedFeatureSucceeds() {
    val featureToTest = FeatureName.CoreFeaturesAndLimits
    val device = runBlocking { requestTestDevice(coreAdapter, intArrayOf(featureToTest)) }
    val deviceFeatures = device.getFeatures()
    assert(deviceFeatures.features.contains(featureToTest)) {
      "Device should have the requested feature: $featureToTest"
    }
    runCatching {
      device.destroy()
    }
  }

  /**
   * Tests that requesting a limit better than what the adapter supports fails.
   *
   * CTSTest: 'limit,better_than_supported'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">WebGPU CTS Test</a>
   */
  @Test
  fun requestDeviceWithBetterThanSupportedLimitFails() {
    val adapterLimits = coreAdapter.getLimits()
    val betterLimit = adapterLimits.maxBindGroups + 1
    assertThrows("Requesting a better limit should fail", DeviceLostException::class.java) {
      runBlocking {
        requestTestDevice(coreAdapter, limits = Limits(maxBindGroups = betterLimit))
      }
    }
  }

  /**
   * Tests that requesting a worse limit than the spec default gets clamped to the default.
   *
   * CTSTest: 'limit,worse_than_default'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">WebGPU CTS Test</a>
   */
  @Test
  fun requestDeviceWithWorseThanDefaultLimitClamps() {
    runBlocking {
      val worseLimit = kCoreLimits.getValue("maxBindGroups") - 1
      assert(worseLimit > 0) // Ensure the value is still valid, just worse.
      val device =
        runBlocking { requestTestDevice(coreAdapter, limits = Limits(maxBindGroups = worseLimit)) }
      val deviceLimits = device.getLimits()
      assertEquals(
        "Device limit should be clamped to the default",
        kCoreLimits.getValue("maxBindGroups"),
        deviceLimits.maxBindGroups
      )
      runCatching { device.destroy() }
    }
  }

  /**
   * Tests that requesting a limit with a value that is not a power of two fails.
   *
   * CTSTest: 'limit,out_of_range'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">WebGPU CTS Test</a>
   *
   */
  @Test
  fun requestDeviceWithInvalidAlignmentLimitFails() {
    // minUniformBufferOffsetAlignment must be a power of 2. 255 is not.
    val invalidAlignment = 255
    assertThrows(
      "Alignment limit not a power of 2 should fail", DeviceLostException::class.java
    ) {
      runBlocking {
        requestTestDevice(
          coreAdapter,
          limits = Limits(
            minUniformBufferOffsetAlignment = invalidAlignment
          )
        )
      }
    }
  }
}
