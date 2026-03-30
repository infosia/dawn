package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@SmallTest
class WebGpuCtsCreateViewTest {
    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp() = runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    private suspend fun doCreateViewTest(
        success: Boolean,
        textureDesc: GPUTextureDescriptor,
        viewDesc: GPUTextureViewDescriptor? = null
    ) {
        val texture = device.createTexture(textureDesc)
        device.pushErrorScope(ErrorFilter.Validation)
        var errorThrown = false
        try {
            if (viewDesc != null) {
                texture.createView(viewDesc)
            } else {
                texture.createView()
            }
            try {
                device.popErrorScope()
            } catch (e: androidx.webgpu.ValidationException) {
                errorThrown = true
            }
        } catch (e: androidx.webgpu.ValidationException) {
            errorThrown = true
            try { device.popErrorScope() } catch (ignored: Exception) {}
        } finally {
            texture.destroy()
        }

        if (success) {
            assert(!errorThrown) { "Expected success but got validation error" }
        } else {
            assert(errorThrown) { "Expected validation error but got success" }
        }
    }

    @Test
    fun testDimension() = runBlocking {
        val cases = listOf(
            Triple(TextureDimension._1D, TextureViewDimension._1D, true),
            Triple(TextureDimension._1D, TextureViewDimension._2D, false),
            Triple(TextureDimension._2D, TextureViewDimension._2DArray, true),
            Triple(TextureDimension._2D, TextureViewDimension.Cube, true),
            Triple(TextureDimension._3D, TextureViewDimension._3D, true)
        )

        for ((texDim, viewDim, expectedSuccess) in cases) {
            val size = if (texDim == TextureDimension._1D) {
                GPUExtent3D(width = 4, height = 1, depthOrArrayLayers = 1)
            } else {
                GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 6)
            }
            val texDesc = GPUTextureDescriptor(
                format = TextureFormat.RGBA8Unorm,
                dimension = texDim,
                size = size,
                usage = TextureUsage.TextureBinding
            )
            val viewDesc = GPUTextureViewDescriptor(dimension = viewDim)
            doCreateViewTest(expectedSuccess, texDesc, viewDesc)
        }
    }

    @Test
    fun testAspect() = runBlocking {
        // "depth-only" is only allowed on depth and depth-stencil formats.
        // "stencil-only" is only allowed on stencil and depth-stencil.
        // "all" is allowed on anything.
        val cases = listOf(
            Triple(TextureFormat.RGBA8Unorm, TextureAspect.All, true),
            Triple(TextureFormat.RGBA8Unorm, TextureAspect.DepthOnly, false),
            Triple(TextureFormat.Depth32Float, TextureAspect.DepthOnly, true),
            Triple(TextureFormat.Depth32Float, TextureAspect.StencilOnly, false)
        )

        for ((format, aspect, expectedSuccess) in cases) {
            val texDesc = GPUTextureDescriptor(
                format = format,
                size = GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 1),
                usage = TextureUsage.TextureBinding
            )
            val viewDesc = GPUTextureViewDescriptor(aspect = aspect)
            doCreateViewTest(expectedSuccess, texDesc, viewDesc)
        }
    }

    @Test
    fun testCubeFacesSquare() = runBlocking {
        val cases = listOf(
            Triple(TextureViewDimension._2D, intArrayOf(4, 5, 6), true),
            Triple(TextureViewDimension.Cube, intArrayOf(4, 4, 6), true),
            Triple(TextureViewDimension.Cube, intArrayOf(4, 5, 6), false)
        )

        for ((viewDim, sizeArr, expectedSuccess) in cases) {
            val size = GPUExtent3D(width = sizeArr[0], height = sizeArr[1], depthOrArrayLayers = sizeArr[2])
            val texDesc = GPUTextureDescriptor(
                format = TextureFormat.RGBA8Unorm,
                size = size,
                usage = TextureUsage.TextureBinding
            )
            val viewDesc = GPUTextureViewDescriptor(dimension = viewDim)
            doCreateViewTest(expectedSuccess, texDesc, viewDesc)
        }
    }

    @Test
    fun testArrayLayersBounds() = runBlocking {
        val texDesc = GPUTextureDescriptor(
            format = TextureFormat.RGBA8Unorm,
            dimension = TextureDimension._2D,
            size = GPUExtent3D(width = 4, height = 4, depthOrArrayLayers = 6),
            usage = TextureUsage.TextureBinding
        )

        // Valid
        doCreateViewTest(true, texDesc, GPUTextureViewDescriptor(
            baseArrayLayer = 0, arrayLayerCount = 6, dimension = TextureViewDimension._2DArray
        ))
        // Out of bounds
        doCreateViewTest(false, texDesc, GPUTextureViewDescriptor(
            baseArrayLayer = 4, arrayLayerCount = 3, dimension = TextureViewDimension._2DArray
        ))
    }

    @Test
    fun testMipLevelsBounds() = runBlocking {
        val texDesc = GPUTextureDescriptor(
            format = TextureFormat.RGBA8Unorm,
            dimension = TextureDimension._2D,
            size = GPUExtent3D(width = 32, height = 32, depthOrArrayLayers = 1),
            mipLevelCount = 4,
            usage = TextureUsage.TextureBinding
        )

        // Valid
        doCreateViewTest(true, texDesc, GPUTextureViewDescriptor(
            baseMipLevel = 0, mipLevelCount = 4
        ))
        
        // Out of bounds
        doCreateViewTest(false, texDesc, GPUTextureViewDescriptor(
            baseMipLevel = 2, mipLevelCount = 3
        ))
    }
}
