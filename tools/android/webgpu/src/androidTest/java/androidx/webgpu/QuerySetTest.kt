package androidx.webgpu

import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import java.nio.ByteOrder
import java.util.concurrent.Executor
import junit.framework.TestCase
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertThrows
import org.junit.Assume
import org.junit.Before
import org.junit.Test

@Suppress("UNUSED_VARIABLE")
@SmallTest
class QuerySetTest {
  private lateinit var webGpu: WebGpu
  private lateinit var device: GPUDevice

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
    webGpu.close()
  }

  companion object {
    private const val QUERY_COUNT = 2
  }


  /** Helper to create a destination buffer for resolveQuerySet */
  private fun createResolveBuffer(size: Long): GPUBuffer {
    return device.createBuffer(
      BufferDescriptor(
        size = size,
        usage = BufferUsage.QueryResolve or BufferUsage.CopySrc  // CopySrc for potential readback.
      )
    )
  }

  @Test
  fun testCreateOcclusionQuerySet() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    TestCase.assertNotNull(querySet)
    assertEquals(QueryType.Occlusion, querySet.type)
    assertEquals(QUERY_COUNT, querySet.count)
  }

  @Test
  fun testCreateTimestampQuerySet() {
    // Timestamp query requires a specific feature.
    if (!device.hasFeature(FeatureName.TimestampQuery)) {
      Assume.assumeTrue("testCreateTimestampQuerySet: TimestampQuery feature not supported.", true)
      return  // Skip test if feature not available.
    }

    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Timestamp, count = QUERY_COUNT)
    )
    TestCase.assertNotNull(querySet)
    assertEquals(QueryType.Timestamp, querySet.type)
    assertEquals(QUERY_COUNT, querySet.count)
    querySet.destroy()
  }

  @Test
  fun testCreateQuerySetWithNegativeCountFails() {
    // Attempting to create a QuerySet with count -1 should fail validation.
    device.pushErrorScope(ErrorFilter.Validation)
    val unusedQuerySet =
      device.createQuerySet(QuerySetDescriptor(type = QueryType.Occlusion, count = -1))
    assertThrows(ValidationException::class.java) {
      runBlocking { device.popErrorScope() }
    }
  }

  @Test
  fun testGetCount() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )

    assertEquals(QUERY_COUNT, querySet.count)
  }

  @Test
  fun testGetType() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    assertEquals(QueryType.Occlusion, querySet.type)
    querySet.destroy()

    if (!device.hasFeature(FeatureName.TimestampQuery)) {
      Assume.assumeTrue("testGetType: TimestampQuery feature not supported.", true)
      return  // Skip test if feature not available.
    }
    val tsQuerySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Timestamp, count = QUERY_COUNT)
    )
    assertEquals(QueryType.Timestamp, tsQuerySet.type)
  }


  @Test
  fun testResolveQuerySetValid() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    // Each individual query result is stored as a 64-bit unsigned integer.
    val destinationBuffer = createResolveBuffer((QUERY_COUNT * Long.SIZE_BYTES).toLong())

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, QUERY_COUNT, destinationBuffer, 0)

    device.pushErrorScope(ErrorFilter.Validation)
    val unusedCommandBuffer = encoder.finish()
    val error = runBlocking { device.popErrorScope() }

    assertEquals(ErrorType.NoError, error)

    querySet.destroy()
    destinationBuffer.destroy()
  }

  @Test
  fun testResolveQuerySetInvalidDestinationUsage() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    // Create buffer *without* QueryResolve usage.
    val invalidBuffer = device.createBuffer(
      BufferDescriptor(size = 8, usage = BufferUsage.CopySrc)
    )

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, QUERY_COUNT, invalidBuffer, 0)  // Invalid usage.

    device.pushErrorScope(ErrorFilter.Validation)
    val unusedCommandBuffer = encoder.finish()
    assertThrows(ValidationException::class.java) {
      runBlocking { device.popErrorScope() }
    }
  }

  @Test
  fun testResolveQuerySetDestinationTooSmall() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    // GPUBuffer only has space for 1 result (8 bytes), but we try to resolve 2.
    val smallBuffer = createResolveBuffer((1 * Long.SIZE_BYTES).toLong())

    val encoder = device.createCommandEncoder()
    encoder.resolveQuerySet(querySet, 0, QUERY_COUNT, smallBuffer, 0)  // Invalid size.

    device.pushErrorScope(ErrorFilter.Validation)
    val unusedCommandBuffer = encoder.finish()
    assertThrows(ValidationException::class.java) {
      runBlocking { device.popErrorScope() }
    }
  }

  @Test
  fun testResolveQuerySetIndexOutOfBounds() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )
    // Each individual query result is stored as a 64-bit unsigned integer.
    val destinationBuffer = createResolveBuffer(QUERY_COUNT * 8L)

    val encoder = device.createCommandEncoder()
    // Try to resolve starting at index 1, count 2 (goes past end).
    encoder.resolveQuerySet(querySet, 1, QUERY_COUNT, destinationBuffer, 0)  // Invalid range.

    device.pushErrorScope(ErrorFilter.Validation)
    val unusedCommandBuffer = encoder.finish()
    assertThrows(ValidationException::class.java) {
      runBlocking { device.popErrorScope() }
    }
  }

  @Test
  fun testResolveQuerySetOffsetAlignment() {
    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = QUERY_COUNT)
    )

    // Each individual query result is stored as a 64-bit unsigned integer.
    val destinationBuffer = createResolveBuffer(QUERY_COUNT * 8L + 8L)

    val encoder = device.createCommandEncoder()
    // Try to resolve starting at offset 4 (invalid alignment).
    encoder.resolveQuerySet(querySet, 0, QUERY_COUNT, destinationBuffer, 4)  // Invalid offset.

    device.pushErrorScope(ErrorFilter.Validation)
    val unusedCommandBuffer = encoder.finish()
    assertThrows(ValidationException::class.java) {
      runBlocking { device.popErrorScope() }
    }
  }

  @Test
  fun testResolveQuerySetAndReadback() = runBlocking {
    val renderTarget = device.createTexture(
      TextureDescriptor(
        size = Extent3D(1, 1, 1),
        format = TextureFormat.RGBA8Unorm,
        usage = TextureUsage.RenderAttachment
      )
    )
    val renderTargetView = renderTarget.createView()

    val renderTargetDepth = device.createTexture(
      TextureDescriptor(
        size = Extent3D(1, 1, 1),
        format = TextureFormat.Depth24Plus,
        usage = TextureUsage.RenderAttachment
      )
    )
    val renderTargetDepthView = renderTargetDepth.createView()

    val queryCount = 1

    val querySet = device.createQuerySet(
      QuerySetDescriptor(type = QueryType.Occlusion, count = queryCount)
    )

    // Usage must include QUERY_RESOLVE (to accept the result) and COPY_SRC (to move data out).
    val resolveBufferSize = (queryCount * Long.SIZE_BYTES).toLong()
    val resolveBuffer = device.createBuffer(
      BufferDescriptor(
        size = resolveBufferSize,
        usage = BufferUsage.QueryResolve or BufferUsage.CopySrc
      )
    )

    val readbackBuffer = device.createBuffer(
      BufferDescriptor(
        size = resolveBufferSize,
        usage = BufferUsage.CopyDst or BufferUsage.MapRead
      )
    )

    val encoder = device.createCommandEncoder()

    // We need a depth stencil attachment for occlusion queries to work.
    val passEncoder = encoder.beginRenderPass(
      RenderPassDescriptor(
        colorAttachments = arrayOf(
          RenderPassColorAttachment(
            view = renderTargetView,
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store,
            clearValue = Color(0.0, 0.0, 0.0, 1.0)
          )
        ),
        depthStencilAttachment = RenderPassDepthStencilAttachment(
          view = renderTargetDepthView,
          depthLoadOp = LoadOp.Clear,
          depthStoreOp = StoreOp.Store,
          depthClearValue = 1.0f
        ),
        occlusionQuerySet = querySet
      )
    )

    passEncoder.beginOcclusionQuery(0)
    // We don't draw anything, so the expected result is 0.
    // If we drew something that passed depth testing, it would be > 0.
    passEncoder.endOcclusionQuery()
    passEncoder.end()

    encoder.resolveQuerySet(querySet, 0, queryCount, resolveBuffer, 0)

    // encoder.copyBufferToBuffer(resolveBuffer, 0, readbackBuffer, 0, resolveBufferSize)

    val commandBuffer = encoder.finish()

    device.queue.submit(arrayOf(commandBuffer))
    device.queue.onSubmittedWorkDone()

    readbackBuffer.mapAsync(MapMode.Read, 0, resolveBufferSize)

    val mappedBuffer = readbackBuffer.getConstMappedRange(size = resolveBufferSize)
    val result = mappedBuffer.order(ByteOrder.nativeOrder()).getLong(0)

    // Since we drew nothing, the occlusion count should be 0.
    // This verifies the resolve operation successfully wrote a 64-bit integer to the buffer.
    assertEquals(0L, result)

    readbackBuffer.unmap()
    readbackBuffer.destroy()
    resolveBuffer.destroy()
    querySet.destroy()
    renderTarget.destroy()
    renderTargetDepth.destroy()
  }
}