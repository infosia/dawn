package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsImageCopyLayoutTest : WebGpuCtsImageCopyBaseTest() {

    @Test
    fun testBufferSizeOutOfBounds() {
        val size = GPUExtent3D(width = 8, height = 8, depthOrArrayLayers = 1)
        val copyLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        
        // RGBA8Unorm, 8x8 requires 256 * 7 + 8 * 4 = 1824 bytes
        // Layout specifies 256 bytes per row, height is 8.
        // We allocate 1024 bytes safely inside buffer, but layout dictates 1824 bytes.
        val undersizedBuffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        val resizedBuffer = device.createBuffer(GPUBufferDescriptor(size = 2048, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        
        val texture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst or TextureUsage.CopySrc,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        testBuffer(resizedBuffer, texture, copyLayout, size, Method.CopyB2T, 2048L, success = true)
        testBuffer(undersizedBuffer, texture, copyLayout, size, Method.CopyB2T, 1024L, success = false)
    }

    @Test
    fun testRowsPerImageValidation() {
        val size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 2)
        
        // Depth = 2 means rowsPerImage must be >= height (2) AND defined. 
        // Note: For depth > 1, undefined or too small rowsPerImage fails.
        val validLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256, rowsPerImage = 2)
        val validLargerLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256, rowsPerImage = 4)
        val invalidSmallLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256, rowsPerImage = 1)
        
        // 256 * 2 (rows) * 2 (depth) = 1024 is technically enough space
        // for valid larger layout: 256 * 4 * 2 = 2048
        val validBuffer = device.createBuffer(GPUBufferDescriptor(size = 4096, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        
        val texture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst or TextureUsage.CopySrc,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        testBuffer(validBuffer, texture, validLayout, size, Method.CopyT2B, 4096L, success = true)
        testBuffer(validBuffer, texture, validLargerLayout, size, Method.CopyT2B, 4096L, success = true)
        testBuffer(validBuffer, texture, invalidSmallLayout, size, Method.CopyT2B, 4096L, success = false)
    }
}
