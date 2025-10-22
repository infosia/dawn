package androidx.webgpu

import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import java.util.concurrent.Executor
import junit.framework.TestCase
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test

@SmallTest
class QuerySetTest {
  private var webGpu: WebGpu? = null
  private lateinit var device: Device

  @Before
  fun setup() = runBlocking {
    val gpu = createWebGpu(
      deviceDescriptor = DeviceDescriptor(
        requiredFeatures = intArrayOf(FeatureName.TimestampQuery),
        deviceLostCallbackExecutor = Executor(Runnable::run),
        deviceLostCallback = null,
        uncapturedErrorCallbackExecutor = Executor(Runnable::run),
        uncapturedErrorCallback = null
      ),
    )  // Request timestamp feature if available.
    webGpu = gpu
    device = gpu.device
  }

  @After
  fun teardown() {
    runCatching { device.destroy() }
    webGpu?.close()
    webGpu = null
  }


  /** Helper to create a destination buffer for resolveQuerySet */
  private fun createResolveBuffer(size: Long): Buffer {
    return device.createBuffer(
      BufferDescriptor(
        size = size,
        usage = BufferUsage.QueryResolve or BufferUsage.CopySrc  // CopySrc for potential readback.
      )
    )
  }

  @Test
  fun testCreateOcclusionQuerySet() {
    val queryCount = 4
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )
    TestCase.assertNotNull(querySet)
    assertEquals(QueryType.Occlusion, querySet.type)
    assertEquals(queryCount, querySet.count)
  }

  @Test
  fun testCreateTimestampQuerySet() {
    // Timestamp query requires a specific feature.
    if (!device.hasFeature(FeatureName.TimestampQuery)) {
      println("Skipping testCreateTimestampQuerySet: TimestampQuery feature not supported.")
      return  // Skip test if feature not available.
    }

    val queryCount = 2
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Timestamp, count = queryCount)
    )
    TestCase.assertNotNull(querySet)
    assertEquals(QueryType.Timestamp, querySet.type)
    assertEquals(queryCount, querySet.count)
    querySet.destroy()
  }

  @Test
  fun testCreateQuerySetWithNegativeCountFails() {
    // Attempting to create a QuerySet with count -1 should fail validation.
    device.pushErrorScope(ErrorFilter.Validation)
    device.createQuerySet(QuerySetDescriptor(type = QueryType.Occlusion, count = -1))
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.Validation, errorScope.type)
  }

  @Test
  fun testGetCount() {
    val queryCount = 8
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )

    assertEquals(queryCount, querySet.count)
  }

  @Test
  fun testGetType() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = 1)
    )
    assertEquals(QueryType.Occlusion, querySet.type)
    querySet.destroy()

    // Test timestamp type if supported.
    if (device.hasFeature(FeatureName.TimestampQuery)) {
      val tsQuerySet = device.createQuerySet(
        QuerySetDescriptor(type = QueryType.Timestamp, count = 1)
      )
      assertEquals(QueryType.Timestamp, tsQuerySet.type)
    }
  }


  @Test
  fun testResolveQuerySetValid() {
    val queryCount = 2
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )
    val destinationBuffer = createResolveBuffer(queryCount * 8L)

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, queryCount, destinationBuffer, 0)

    device.pushErrorScope(ErrorFilter.Validation)
    encoder.finish()
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.NoError, errorScope.type)

    querySet.destroy()
    destinationBuffer.destroy()
  }

  @Test
  fun testResolveQuerySetInvalidDestinationUsage() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = 1)
    )
    // Create buffer *without* QueryResolve usage.
    val invalidBuffer = device.createBuffer(
      BufferDescriptor(size = 8, usage = BufferUsage.CopySrc)
    )

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, 1, invalidBuffer, 0)  // Invalid usage.

    device.pushErrorScope(ErrorFilter.Validation)
    encoder.finish()
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.Validation, errorScope.type)
  }

  @Test
  fun testResolveQuerySetDestinationTooSmall() {
    val queryCount = 2
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )
    // Buffer only has space for 1 result (8 bytes), but we try to resolve 2.
    val smallBuffer = createResolveBuffer(8L)

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, queryCount, smallBuffer, 0)  // Invalid size.

    device.pushErrorScope(ErrorFilter.Validation)
    encoder.finish()
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.Validation, errorScope.type)
  }

  @Test
  fun testResolveQuerySetIndexOutOfBounds() {
    val queryCount = 2
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )
    val destinationBuffer = createResolveBuffer(queryCount * 8L)

    val encoder = device.createCommandEncoder()
    // Try to resolve starting at index 1, count 2 (goes past end).
    encoder.resolveQuerySet(querySet, 1, queryCount, destinationBuffer, 0)  // Invalid range.

    device.pushErrorScope(ErrorFilter.Validation)
    encoder.finish()
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.Validation, errorScope.type)
  }

  @Test
  fun testResolveQuerySetOffsetAlignment() {
    val queryCount = 2
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )
    val destinationBuffer = createResolveBuffer(queryCount * 8L + 8L)

    val encoder = device.createCommandEncoder()
    // Try to resolve starting at offset 4 (invalid alignment)
    encoder.resolveQuerySet(querySet, 0, queryCount, destinationBuffer, 4)  // Invalid offset.

    device.pushErrorScope(ErrorFilter.Validation)
    encoder.finish()
    val errorScope = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.Validation, errorScope.type)
  }
}