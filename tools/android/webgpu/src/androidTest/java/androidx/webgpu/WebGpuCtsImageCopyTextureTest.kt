package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsImageCopyTextureTest : WebGpuCtsImageCopyBaseTest() {

    @Test
    fun testTextureUsageFlags() {
        val size = GPUExtent3D(width = 2, height = 2, depthOrArrayLayers = 1)
        val copyLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        
        val buffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        
        val validDstTexture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )
        val validSrcTexture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopySrc,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )
        val invalidTexture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.TextureBinding,
                size = size
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        // B2T requires CopyDst on texture
        testBuffer(buffer, validDstTexture, copyLayout, size, Method.CopyB2T, 1024L, success = true)
        testBuffer(buffer, invalidTexture, copyLayout, size, Method.CopyB2T, 1024L, success = false)

        // T2B requires CopySrc on texture
        testBuffer(buffer, validSrcTexture, copyLayout, size, Method.CopyT2B, 1024L, success = true)
        testBuffer(buffer, invalidTexture, copyLayout, size, Method.CopyT2B, 1024L, success = false)
    }

    @Test
    fun testCopyOutOfBounds() {
        val textureSize = GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 1)
        
        // Exact size matches texture boundaries
        val validExtent = GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 1)
        // Trying to copy 5 pixels wide out of a 4 pixel wide texture
        val outOfBoundsExtent = GPUExtent3D(width = 5, height = 4, depthOrArrayLayers = 1)

        val copyLayout = GPUTexelCopyBufferLayout(bytesPerRow = 256)
        
        val buffer = device.createBuffer(GPUBufferDescriptor(size = 1024, usage = BufferUsage.CopySrc or BufferUsage.CopyDst))
        
        val texture = device.createTexture(
            GPUTextureDescriptor.Builder(
                usage = TextureUsage.CopyDst or TextureUsage.CopySrc,
                size = textureSize
            ).setFormat(TextureFormat.RGBA8Unorm).build()
        )

        // B2T
        testBuffer(buffer, texture, copyLayout, validExtent, Method.CopyB2T, 1024L, success = true)
        testBuffer(buffer, texture, copyLayout, outOfBoundsExtent, Method.CopyB2T, 1024L, success = false)

        // T2B
        testBuffer(buffer, texture, copyLayout, validExtent, Method.CopyT2B, 1024L, success = true)
        testBuffer(buffer, texture, copyLayout, outOfBoundsExtent, Method.CopyT2B, 1024L, success = false)
    }
}
