package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.DeviceLostException
import androidx.webgpu.helper.UncapturedErrorException
import androidx.webgpu.helper.initLibrary
import junit.framework.TestCase
import junit.framework.TestCase.assertNotNull
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertThrows
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class AdapterTest {
  private lateinit var instance: Instance
  private lateinit var adapter: Adapter

  @Before
  fun setup() = runBlocking {
    initLibrary()
    val inst = createInstance()
    instance = inst

    val (status, adapt, _) = inst.requestAdapter()
    TestCase.assertEquals(RequestAdapterStatus.Success, status)
    assertNotNull(adapt)
    adapter = adapt!!
  }

  @After
  fun teardown() {
    adapter.close()
    instance.close()
  }

  /**
   * Helper map for default limit values as defined by the WebGPU spec.
   * A full implementation would have all limits.
   */
  companion object {
    val kDefaultLimits = mapOf(
      "maxTextureDimension2D" to 8192,
      "maxBindGroups" to 4,
      "minUniformBufferOffsetAlignment" to 256
    )
  }

  @Test
  fun adaptorTest() {
    val instance = createInstance()
    runBlocking {
      val result = instance.requestAdapter()
      val adapter = result.adapter
      check(result.status == RequestAdapterStatus.Success && adapter != null) {
        result.message
      }
      val adapterInfo = adapter.getInfo()

      assertEquals(
        "The backend type should be Vulkan",
        BackendType.Vulkan, adapterInfo.backendType
      )
    }
  }

  private suspend fun getDevice(
    featureToTest: IntArray = intArrayOf(),
    limits: Limits = Limits(),
  ): Device {
    val descriptor = DeviceDescriptor(
      requiredFeatures = featureToTest,
      requiredLimits = limits,
      deviceLostCallback = DeviceLostCallback { device, reason, message ->
        throw DeviceLostException(device, reason, message)
      },
      uncapturedErrorCallback = UncapturedErrorCallback { device, type, message ->
        throw UncapturedErrorException(device, type, message)
      }

    )
    val (status, device) = adapter.requestDevice(descriptor)
    TestCase.assertEquals(RequestDeviceStatus.Success, status)
    assertNotNull(device)
    return device!!
  }

  /**
   * Tests requesting a device with default parameters.
   *
   * CTS Test: requestAdapter_no_parameters
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
   */

  @Test
  fun requestDeviceWithDefaultParameters() {
    val (status, device, _) = runBlocking { adapter.requestDevice() }
    TestCase.assertEquals(RequestDeviceStatus.Success, status)
    assertNotNull(device)

    val features = device!!.getFeatures().features

    val featureCheckPassed = when (features.size) {
      0 -> true // Having no features is valid.
      1 -> features.first() == FeatureName.CoreFeaturesAndLimits // One feature is ok if it's the right one.
      else -> false // Any other number of features is an error.
    }
    try {
      assert(featureCheckPassed) {
        "Default device should have 0 or 1 ('CoreFeaturesAndLimits') features, but found: $features"
      }

      // Check a few key limits against their default values.
      val limits = device.getLimits()
      TestCase.assertEquals(
        "maxTextureDimension2D should be default",
        kDefaultLimits["maxTextureDimension2D"],
        limits.maxTextureDimension2D
      )
      TestCase.assertEquals(
        "maxBindGroups should be default", kDefaultLimits["maxBindGroups"], limits.maxBindGroups
      )
    } finally {
      device.destroy()
    }
  }

  /**
   * Tests that an adapter can only grant a device once. Subsequent requests fail.
   *
   * CTS Test: requestAdapter_stale
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
   */
  @Test
  fun requestDeviceFailsAfterSuccessfulRequest() {
    val instance = createInstance()
    val (_, adapter, _) = runBlocking { instance.requestAdapter() }
    assertNotNull(adapter)
    val (status, device, _) = runBlocking { adapter!!.requestDevice() }
    TestCase.assertEquals(RequestDeviceStatus.Success, status)
    assertNotNull(device)
    device!!.destroy()
    assertThrows(
      "Adapter should be consumed after one device request", IllegalStateException::class.java
    ) {
      runBlocking {
        val (status, _, message) = adapter!!.requestDevice()
        check(status == RequestDeviceStatus.Success) { message }
      }
    }
  }

  /**
   * Tests requesting a device with a feature that the adapter supports.
   *
   * CTSTest: features,known
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
   */
  @Test
  fun requestDeviceWithSupportedFeatureSucceeds() {
    val featureToTest = FeatureName.CoreFeaturesAndLimits
    val device = runBlocking { getDevice(intArrayOf(featureToTest)) }

    val deviceFeatures = device.getFeatures()
    assumeTrue(
      "Device should have the requested feature: $featureToTest",
      deviceFeatures.features.contains(featureToTest)
    )
    runCatching {
      device.destroy()
    }
  }

  /**
   * Tests that requesting a limit better than what the adapter supports fails.
   *
   * CTSTest: 'limit,better_than_supported'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
   */
  @Test
  fun requestDeviceWithBetterThanSupportedLimitFails() {
    val adapterLimits = adapter.getLimits()
    val betterLimit = adapterLimits.maxBindGroups + 1

    assertThrows("Requesting a better limit should fail", DeviceLostException::class.java) {
      runBlocking {
        getDevice(limits = Limits(maxBindGroups = betterLimit))
      }
    }
  }

  /**
   * Tests that requesting a worse limit than the spec default gets clamped to the default.
   *
   * CTSTest: 'limit,worse_than_default'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
   */
  @Test
  fun requestDeviceWithWorseThanDefaultLimitClamps() {
    runBlocking {
      val worseLimit = kDefaultLimits["maxBindGroups"]!! - 1
      assert(worseLimit > 0) // Ensure the value is still valid, just worse.

      val device = runBlocking { getDevice(limits = Limits(maxBindGroups = worseLimit)) }
      val deviceLimits = device.getLimits()
      TestCase.assertEquals(
        "Device limit should be clamped to the default",
        kDefaultLimits["maxBindGroups"]!!,
        deviceLimits.maxBindGroups
      )
      runCatching { device.destroy() }
    }
  }

  /**
   * Tests that requesting a limit with a value that is not a power of two fails.
   *
   * CTSTest: 'limit,out_of_range'
   * @see <a href="https://github.com/gpuweb/cts/blob/main/src/webgpu/api/operation/adapter/requestAdapter.spec.ts">Original TS Test</a>
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
        getDevice(limits = Limits(minUniformBufferOffsetAlignment = invalidAlignment))
      }
    }
  }
}
