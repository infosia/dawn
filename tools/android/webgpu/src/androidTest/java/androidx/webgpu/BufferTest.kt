package androidx.webgpu

import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.UncapturedErrorException
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertThrows
import org.junit.Test
import junit.framework.TestCase
import org.junit.After
import org.junit.Before
import org.junit.Assert.assertArrayEquals

class BufferTest {
  private lateinit var device: Device
  private lateinit var webGpu: WebGpu

  @Before
  fun setup() = runBlocking {
    webGpu = createWebGpu()
    device = webGpu.device
  }

  @After
  fun teardown() {
    runCatching { device.destroy() }
    webGpu.close()

  }

  @SmallTest
  @Test
    /**
     * Test that calling getMappedRange() on a mapped buffer does not raise an exception.
     */
  fun bufferMapTest() {
    device.createBuffer(
      BufferDescriptor(
        usage = BufferUsage.Vertex,
        size = 1024,
        mappedAtCreation = true
      )
    ).apply {
      getMappedRange(size = size)
    }
  }

  @SmallTest
  @Test
    /**
     * Test that calling getMappedRange() on a non-mapped buffer raises an exception.
     */
  fun bufferMapFailureTest() {
    assertThrows(Error::class.java) {
      device.createBuffer(
        BufferDescriptor(
          usage = BufferUsage.Vertex,
          size = 1024,
          mappedAtCreation = false
        )
      ).apply {
        getMappedRange(size = size)
      }
    }
  }

  @SmallTest
  @Test
    /**
     * Tests that the size and usage properties of a buffer are correct.
     */
  fun testBufferSizeAndUsage() {
    val bufferSize = 256L
    val bufferUsage = BufferUsage.Vertex or BufferUsage.CopyDst

    val buffer = device.createBuffer(
      BufferDescriptor(
        usage = bufferUsage,
        size = bufferSize
      )
    )

    TestCase.assertEquals(bufferSize, buffer.size)
    TestCase.assertEquals(bufferUsage, buffer.usage)
  }

  @SmallTest
  @Test
    /**
     * Tests that a buffer is no longer usable after being destroyed.
     */
  fun testDestroy() {
    val buffer = device.createBuffer(
      BufferDescriptor(
        usage = BufferUsage.MapRead,
        size = 16
      )
    )

    buffer.destroy()

    // After destroying the buffer, further operations should fail.
    assertThrows(UncapturedErrorException::class.java) {
      runBlocking {
        buffer.mapAsync(MapMode.Read, 0, 16)
      }
    }
  }

  @MediumTest
  @Test
    /**
     * Tests the full buffer mapping and unmapping lifecycle.
     */
  fun testMapAsyncUnmap() {
    val bufferSize = 16L
    val buffer = device.createBuffer(
      BufferDescriptor(
        usage = BufferUsage.MapRead or BufferUsage.CopyDst,
        size = bufferSize,
      )
    )

    TestCase.assertEquals(BufferMapState.Unmapped, buffer.mapState)

    val mapResult = runBlocking { buffer.mapAsync(MapMode.Read, 0, bufferSize) }
    TestCase.assertEquals(MapAsyncStatus.Success, mapResult.status)

    TestCase.assertEquals(BufferMapState.Mapped, buffer.mapState)

    buffer.unmap()

    TestCase.assertEquals(BufferMapState.Unmapped, buffer.mapState)

    // Should not be able to get the mapped range after unmapping.
    assertThrows(Error::class.java) {
      buffer.getMappedRange(0, bufferSize)
    }
  }

  /**
   * Verifies the data integrity of a full CPU -> GPU -> CPU round trip.
   */
  @Test
  @MediumTest
  fun testWriteAndReadBuffer() {
    val queue = device.getQueue()

    val cpuData = floatArrayOf(1.1f, 2.2f, 3.3f, 4.4f, 5.5f)
    val bufferSize = cpuData.size * Float.SIZE_BYTES * 1L

    val cpuDataBuffer = ByteBuffer.allocateDirect(bufferSize.toInt()).order(ByteOrder.nativeOrder())
      .apply { asFloatBuffer().put(cpuData).rewind() }

    val gpuWriteBuffer = device.createBuffer(
      BufferDescriptor(
        size = bufferSize,
        usage = BufferUsage.CopyDst or BufferUsage.CopySrc
      )
    )

    queue.writeBuffer(gpuWriteBuffer, 0, cpuDataBuffer)

    val gpuReadBuffer = device.createBuffer(
      BufferDescriptor(
        size = bufferSize,
        usage = BufferUsage.CopyDst or BufferUsage.MapRead
      )
    )
    try {
      val commandEncoder = device.createCommandEncoder()
      commandEncoder.copyBufferToBuffer(
        source = gpuWriteBuffer,
        sourceOffset = 0,
        destination = gpuReadBuffer,
        destinationOffset = 0,
        size = bufferSize
      )
      val gpuCommand = commandEncoder.finish()
      queue.submit(arrayOf(gpuCommand))
      runBlocking { queue.onSubmittedWorkDone() }

      val mapStatus =
        runBlocking { gpuReadBuffer.mapAsync(mode = MapMode.Read, size = bufferSize, offset = 0) }
      TestCase.assertEquals(mapStatus.status, MapAsyncStatus.Success)

      val arrayBuffer = gpuReadBuffer.getConstMappedRange(size = bufferSize).asFloatBuffer()
      gpuReadBuffer.unmap()
      val floatArray = FloatArray(arrayBuffer.remaining())
      arrayBuffer.get(floatArray)
      val delta = 0.00001f // Define an acceptable tolerance
      assertArrayEquals(
        "The GPU data does not match the original CPU data.",
        cpuData,
        floatArray,
        delta
      )
    } finally {
      gpuReadBuffer.destroy()
      gpuWriteBuffer.destroy()
    }

  }

  /**
   * Verifies that `readMappedRange` successfully reads data from a mapped buffer.
   */
  @Test
  fun readMappedRange_succeedsAndDataIsCorrect() {
    val initialData = floatArrayOf(1f, 2f, 3f, 4f)
    val bufferSize = initialData.size * Float.SIZE_BYTES * 1L

    val byteBuffer = ByteBuffer.allocateDirect(bufferSize.toInt()).order(ByteOrder.nativeOrder())
      .apply { asFloatBuffer().put(initialData).rewind() }

    val buffer = device.createBuffer(
      BufferDescriptor(
        size = bufferSize,
        usage = BufferUsage.MapRead or BufferUsage.CopyDst
      )
    )
    device.getQueue().writeBuffer(buffer, 0, byteBuffer)

    runBlocking {
      buffer.mapAsync(MapMode.Read, offset = 0, size = bufferSize)
    }

    val readByteBuffer =
      ByteBuffer.allocateDirect(bufferSize.toInt()).order(ByteOrder.nativeOrder())
    buffer.readMappedRange(0, readByteBuffer)

    val readByteBufferFloat = readByteBuffer.asFloatBuffer()
    val readData = FloatArray(readByteBufferFloat.remaining())

    readByteBufferFloat.get(readData)

    val delta = 0.00001f // Define an acceptable tolerance
    assertArrayEquals(
      "The GPU data does not match the original CPU data.",
      initialData,
      readData,
      delta
    )
  }

  /**
   * Verifies that `writeMappedRange` fails when the buffer is not mapped.
   */
  @Test
  fun writeMappedRange_failsWhenNotMapped() {
    val buffer = device.createBuffer(
      BufferDescriptor(size = 16, usage = BufferUsage.MapWrite)
    )
    val byteBuffer = ByteBuffer.allocateDirect(16)

    assertThrows(Error::class.java) {
      buffer.writeMappedRange(0, byteBuffer)
    }
  }
}