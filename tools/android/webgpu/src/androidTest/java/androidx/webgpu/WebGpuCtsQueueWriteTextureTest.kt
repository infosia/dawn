package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking
import java.nio.ByteBuffer

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsQueueWriteTextureTest {

    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice
    private lateinit var queue: GPUQueue

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
        queue = device.queue
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    private suspend fun tryWriteTexture(
        success: Boolean,
        destination: GPUTexelCopyTextureInfo,
        data: ByteBuffer,
        writeSize: GPUExtent3D,
        dataLayout: GPUTexelCopyBufferLayout
    ) {
        device.pushErrorScope(ErrorFilter.Validation)
        try {
            queue.writeTexture(destination, data, writeSize, dataLayout)
            device.popErrorScope()
            if (!success) {
                fail("Expected validation error but succeeded")
            }
        } catch (e: Exception) {
            if (success) {
                fail("Expected success but failed with: $e")
            }
        }
    }

    @Test
    fun testWriteTextureValid() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopyDst
            )
        )
        val data = ByteBuffer.allocateDirect(1024) // 256 * 3 + 16 = 784 required bytes
        tryWriteTexture(
            success = true,
            destination = GPUTexelCopyTextureInfo(texture = texture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256, rowsPerImage = 4)
        )
        runCatching { texture.destroy() }
        Unit
    }

    @Test
    fun testWriteTextureDestroyed() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopyDst
            )
        )
        texture.destroy()
        val data = ByteBuffer.allocateDirect(1024)
        tryWriteTexture(
            success = false,
            destination = GPUTexelCopyTextureInfo(texture = texture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        )
        Unit
    }

    @Test
    fun testWriteTextureBytesPerRowAlignment() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopyDst
            )
        )
        val data = ByteBuffer.allocateDirect(1024)
        
        // bytesPerRow < width * bytesPerBlock is invalid
        tryWriteTexture(
            success = false,
            destination = GPUTexelCopyTextureInfo(texture = texture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 15) // width (4) * bytesPerBlock (4) = 16
        )

        // bytesPerRow >= width * bytesPerBlock is valid for writeTexture (unlike copyBufferToTexture)
        tryWriteTexture(
            success = true,
            destination = GPUTexelCopyTextureInfo(texture = texture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 16)
        )

        runCatching { texture.destroy() }
        Unit
    }

    @Test
    fun testWriteTextureUsage() = runBlocking {
        val textureWithoutCopyDst = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.RenderAttachment
            )
        )
        val data = ByteBuffer.allocateDirect(1024)
        tryWriteTexture(
            success = false,
            destination = GPUTexelCopyTextureInfo(texture = textureWithoutCopyDst),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        )
        runCatching { textureWithoutCopyDst.destroy() }
        Unit
    }

    @Test
    fun testWriteTextureOobData() = runBlocking {
        val texture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                usage = TextureUsage.CopyDst
            )
        )
        // Array buffer too small for what bytesPerRow/rowsPerImage expects
        val data = ByteBuffer.allocateDirect(16) // Very small
        tryWriteTexture(
            success = false,
            destination = GPUTexelCopyTextureInfo(texture = texture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256, rowsPerImage = 4)
        )
        runCatching { texture.destroy() }
        Unit
    }

    @Test
    fun testWriteTextureMultisampled() = runBlocking {
        val multisampledTexture = device.createTexture(
            GPUTextureDescriptor(
                size = GPUExtent3D(4, 4, 1),
                format = TextureFormat.RGBA8Unorm,
                sampleCount = 4,
                usage = TextureUsage.CopyDst or TextureUsage.RenderAttachment
            )
        )
        val data = ByteBuffer.allocateDirect(1024)
        tryWriteTexture(
            success = false, // writeTexture is invalid for multisampled textures
            destination = GPUTexelCopyTextureInfo(texture = multisampledTexture),
            data = data,
            writeSize = GPUExtent3D(4, 4, 1),
            dataLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        )
        runCatching { multisampledTexture.destroy() }
        Unit
    }
}
