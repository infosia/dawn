package androidx.webgpu

import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import org.junit.After
import org.junit.Before

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.runBlocking

@RunWith(AndroidJUnit4::class)
@SmallTest

class WebGpuCtsCreateBindGroupTest {
    private lateinit var webGpu: WebGpu
    private lateinit var device: GPUDevice

    @Before
    fun setUp(): Unit = kotlinx.coroutines.runBlocking {
        webGpu = createWebGpu()
        device = webGpu.device
    }

    @After
    fun tearDown() {
        runCatching { device.destroy() }
        webGpu.close()
    }

    private fun doCreateBindGroupTest(
        success: Boolean,
        descriptor: GPUBindGroupDescriptor
    ) = runBlocking {
        device.pushErrorScope(ErrorFilter.Validation)
        var errorThrown = false
        try {
            device.createBindGroup(descriptor)
            try {
                device.popErrorScope()
            } catch (e: androidx.webgpu.ValidationException) {
                errorThrown = true
            }
        } catch (e: androidx.webgpu.ValidationException) {
            errorThrown = true
            try { device.popErrorScope() } catch (ignored: Exception) {}
        }
        if (success) {
            assert(!errorThrown) { "Expected success but got validation error" }
        } else {
            assert(errorThrown) { "Expected validation error but got success" }
        }
    }

    @Test
    fun testBindingCountMismatch(): Unit = runBlocking {
        val cases = listOf(
            Pair(1, 1), Pair(1, 2), Pair(2, 1), Pair(2, 2),
            Pair(3, 1), Pair(1, 3), Pair(3, 3)
        )
        for ((layoutEntryCount, bindGroupEntryCount) in cases) {
            val layoutEntries = mutableListOf<GPUBindGroupLayoutEntry>()
            for (i in 0 until layoutEntryCount) {
                layoutEntries.add(
                    GPUBindGroupLayoutEntry(
                        binding = i,
                        visibility = ShaderStage.Compute,
                        buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage)
                    )
                )
            }
            val layout = device.createBindGroupLayout(
                GPUBindGroupLayoutDescriptor(entries = layoutEntries.toTypedArray())
            )

            val entries = mutableListOf<GPUBindGroupEntry>()
            // Need a storage buffer for tests
            val buffer = device.createBuffer(
                GPUBufferDescriptor(
                    size = 1024,
                    usage = BufferUsage.Storage
                )
            )
            for (i in 0 until bindGroupEntryCount) {
                entries.add(
                    GPUBindGroupEntry(
                        binding = i,
                        buffer = buffer
                    )
                )
            }

            val descriptor = GPUBindGroupDescriptor(
                layout = layout,
                entries = entries.toTypedArray()
            )

            val success = layoutEntryCount == bindGroupEntryCount
            doCreateBindGroupTest(success, descriptor)
        }
    }

    @Test
    fun testBindingMustBePresentInLayout(): Unit = runBlocking {
        val cases = listOf(
            Pair(0, 0), Pair(0, 1), Pair(1, 0), Pair(1, 1), Pair(2, 0), Pair(2, 2)
        )
        for ((layoutBinding, binding) in cases) {
            val layout = device.createBindGroupLayout(
                GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = layoutBinding,
                            visibility = ShaderStage.Compute,
                            buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage)
                        )
                    )
                )
            )

            val buffer = device.createBuffer(
                GPUBufferDescriptor(
                    size = 1024,
                    usage = BufferUsage.Storage
                )
            )

            val descriptor = GPUBindGroupDescriptor(
                layout = layout,
                entries = arrayOf(
                    GPUBindGroupEntry(
                        binding = binding,
                        buffer = buffer
                    )
                )
            )

            val success = layoutBinding == binding
            doCreateBindGroupTest(success, descriptor)
        }
    }

    @Test
    fun testMultisampledValidation(): Unit = runBlocking {
        class TestCase(val multisampled: Boolean, val sampleCount: Int)
        val cases = listOf(
            TestCase(true, 1),
            TestCase(true, 4),
            TestCase(false, 1),
            TestCase(false, 4)
        )
        for (case in cases) {
            val layout = device.createBindGroupLayout(
                GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = 0,
                            visibility = ShaderStage.Compute,
                            texture = GPUTextureBindingLayout(
                                multisampled = case.multisampled,
                                sampleType = if (case.multisampled) TextureSampleType.UnfilterableFloat else TextureSampleType.Float
                            )
                        )
                    )
                )
            )

            val texture = device.createTexture(
                GPUTextureDescriptor(
                    size = GPUExtent3D(width = 16, height = 16, depthOrArrayLayers = 1),
                    format = TextureFormat.RGBA8Unorm,
                    usage = TextureUsage.TextureBinding or TextureUsage.RenderAttachment,
                    sampleCount = case.sampleCount
                )
            )

            val textureView = texture.createView()
            val descriptor = GPUBindGroupDescriptor(
                layout = layout,
                entries = arrayOf(
                    GPUBindGroupEntry(
                        binding = 0,
                        textureView = textureView
                    )
                )
            )

            val success = (!case.multisampled && case.sampleCount == 1) || (case.multisampled && case.sampleCount > 1)
            doCreateBindGroupTest(success, descriptor)
        }
    }

    @Test
    fun testBufferOffsetAndSizeForBindGroupsMatch(): Unit = runBlocking {
        class TestCase(val bindBufferResource: Boolean, val offset: Long, val size: Long?, val success: Boolean)
        val cases = listOf(
            TestCase(false, 0, 512, true),
            TestCase(false, 256, 256, true),
            TestCase(true, 0, null, true),
            TestCase(false, 0, 1024, true),
            TestCase(false, 0, null, true),
            TestCase(false, 256 * 3L, 256, true),
            TestCase(false, 256 * 3L, null, true),
            TestCase(false, 0, 0, false),
            TestCase(false, 256, 0, false),
            TestCase(false, 1024, 0, false),
            TestCase(false, 1024, null, false), // Since offset 1024 on 1024 sized buffer means effectively size 0
            TestCase(false, 1, 256, false),
            TestCase(false, 1, null, false),
            TestCase(false, 127, 256, false),
            TestCase(false, 255, 256, false),
            TestCase(false, 256 * 5L, 0, false),
            TestCase(false, 0, 256 * 5L, false),
            TestCase(false, 1024, 1, false)
        )
        for (case in cases) {
            val layout = device.createBindGroupLayout(
                GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = 0,
                            visibility = ShaderStage.Compute,
                            buffer = GPUBufferBindingLayout(type = BufferBindingType.Storage)
                        )
                    )
                )
            )

            val buffer = device.createBuffer(
                GPUBufferDescriptor(
                    size = 1024,
                    usage = BufferUsage.Storage
                )
            )

            var entry: GPUBindGroupEntry
            if (case.bindBufferResource || (case.offset == 0L && case.size == null)) {
                entry = GPUBindGroupEntry(
                    binding = 0,
                    buffer = buffer
                )
            } else if (case.size == null) {
                entry = GPUBindGroupEntry(
                    binding = 0,
                    
                        buffer = buffer,
                        offset = case.offset
                    
                )
            } else {
                entry = GPUBindGroupEntry(
                    binding = 0,
                    
                        buffer = buffer,
                        offset = case.offset,
                        size = case.size
                    
                )
            }

            val descriptor = GPUBindGroupDescriptor(
                layout = layout,
                entries = arrayOf(entry)
            )

            doCreateBindGroupTest(case.success, descriptor)
        }
    }

    @Test
    fun testMinBindingSize(): Unit = runBlocking {
        class TestCase(val minBindingSize: Long?, val size: Long)
        val cases = mutableListOf<TestCase>()
        for (minBindingSize in listOf<Long?>(null, 4L, 8L, 256L)) {
            if (minBindingSize != null) {
                cases.add(TestCase(minBindingSize, minBindingSize - 4L))
                cases.add(TestCase(minBindingSize, minBindingSize))
                cases.add(TestCase(minBindingSize, minBindingSize + 4L))
            } else {
                cases.add(TestCase(null, 4L))
                cases.add(TestCase(null, 256L))
            }
        }
        
        for (case in cases) {
            val layout = device.createBindGroupLayout(
                GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = 0,
                            visibility = ShaderStage.Compute,
                            buffer = GPUBufferBindingLayout(
                                type = BufferBindingType.Storage,
                                minBindingSize = case.minBindingSize ?: 0L 
                            )
                        )
                    )
                )
            )

            // Make sure the size allocated is bounded accurately so we don't hit creation limits instead of validation
            val allocatedSize = if (case.size < 4L) 4L else case.size
            val buffer = device.createBuffer(
                GPUBufferDescriptor(
                    size = allocatedSize,
                    usage = BufferUsage.Storage
                )
            )
            
            val descriptor = GPUBindGroupDescriptor(
                layout = layout,
                entries = arrayOf(
                    GPUBindGroupEntry(
                        binding = 0,
                        
                            buffer = buffer,
                            offset = 0L,
                            size = case.size
                        
                    )
                )
            )

            val success = case.minBindingSize == null || case.size >= case.minBindingSize
            doCreateBindGroupTest(success, descriptor)
        }
    }
}
