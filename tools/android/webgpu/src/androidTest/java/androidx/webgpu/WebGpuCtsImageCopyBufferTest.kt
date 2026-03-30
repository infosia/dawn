package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsImageCopyBufferTest : WebGpuCtsImageCopyBaseTest() {

    @Test
    fun testBufferUsageFlags() {
        val size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1)
        val copyLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        
        val validSrcBuffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopySrc))
        val validDstBuffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopyDst))
        val invalidBuffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.Storage))

        val texture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst or TextureUsage.CopySrc,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        // B2T validation
        testBuffer(validSrcBuffer, texture, copyLayout, size, Method.CopyB2T, 1024L, success = true)
        testBuffer(invalidBuffer, texture, copyLayout, size, Method.CopyB2T, 1024L, success = false)

        // T2B validation
        testBuffer(validDstBuffer, texture, copyLayout, size, Method.CopyT2B, 1024L, success = true)
        testBuffer(invalidBuffer, texture, copyLayout, size, Method.CopyT2B, 1024L, success = false)
    }

    @Test
    fun testBytesPerRowAlignment() {
        val size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1)
        
        // 256 bytes per row is strictly required for T2B/B2T but 0 can be valid if copying only 1 row!
        // Wait, for height=2, >1 rows are copied, so it must be 256.
        val validLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        val invalidLayout = GPUTexelCopyBufferLayout(bytesPerRow = 255)
        
        val validBuffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        
        val texture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst or TextureUsage.CopySrc,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        // WriteTexture doesn't care about 256-byte alignment bounds!
        testBuffer(validBuffer, texture, validLayout, size, Method.WriteTexture, 1024L, success = true)
        testBuffer(validBuffer, texture, invalidLayout, size, Method.WriteTexture, 1024L, success = true)

        // B2T and T2B enforce 256 alignment for copyExtents.height > 1
        testBuffer(validBuffer, texture, validLayout, size, Method.CopyB2T, 1024L, success = true)
        testBuffer(validBuffer, texture, invalidLayout, size, Method.CopyB2T, 1024L, success = false)

        testBuffer(validBuffer, texture, validLayout, size, Method.CopyT2B, 1024L, success = true)
        testBuffer(validBuffer, texture, invalidLayout, size, Method.CopyT2B, 1024L, success = false)
    }
}
