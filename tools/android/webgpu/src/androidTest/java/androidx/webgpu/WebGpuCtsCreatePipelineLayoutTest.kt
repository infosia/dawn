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

class WebGpuCtsCreatePipelineLayoutTest {
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

    private fun doCreatePipelineLayoutTest(
        success: Boolean,
        descriptor: GPUPipelineLayoutDescriptor
    ) = runBlocking {
        device.pushErrorScope(ErrorFilter.Validation)
        var errorThrown = false
        try {
            device.createPipelineLayout(descriptor)
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
    fun testNumberOfDynamicBuffersExceedsTheMaximumValue(): Unit = runBlocking {
        val visibilities = listOf(0, 2, 4, 6) // ShaderStage matching
        val types = listOf(
            BufferBindingType.Uniform,
            BufferBindingType.Storage,
            BufferBindingType.ReadOnlyStorage
        )
        
        for (visibility in visibilities) {
            for (type in types) {
                // Approximate limits logic equivalent
                val maxDynamicLimit = when (type) {
                    BufferBindingType.Uniform -> device.getLimits().maxDynamicUniformBuffersPerPipelineLayout
                    BufferBindingType.Storage, BufferBindingType.ReadOnlyStorage -> device.getLimits().maxDynamicStorageBuffersPerPipelineLayout
                    else -> 0
                }
                
                // Max limits per specific stage constraints
                val perStageLimit = when (type) {
                    BufferBindingType.Uniform -> device.getLimits().maxUniformBuffersPerShaderStage
                    BufferBindingType.Storage, BufferBindingType.ReadOnlyStorage -> device.getLimits().maxStorageBuffersPerShaderStage
                    else -> 0
                }
                
                val limit = Math.min(maxDynamicLimit, perStageLimit)
                if (limit == 0) continue
                
                // Build a BGL utilizing the limit completely
                val maxDynamicBufferBindings = mutableListOf<GPUBindGroupLayoutEntry>()
                for (binding in 0 until limit) {
                    maxDynamicBufferBindings.add(
                        GPUBindGroupLayoutEntry(
                            binding = binding,
                            visibility = visibility,
                            buffer = GPUBufferBindingLayout(
                                type = type,
                                hasDynamicOffset = true
                            )
                        )
                    )
                }
                val maxBgl = device.createBindGroupLayout(
                    GPUBindGroupLayoutDescriptor(entries = maxDynamicBufferBindings.toTypedArray())
                )
                
                // A good descriptor has no dynamic offsets
                val goodDescriptor = GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = 0,
                            visibility = visibility,
                            buffer = GPUBufferBindingLayout(
                                type = type,
                                hasDynamicOffset = false
                            )
                        )
                    )
                )
                val goodBgl = device.createBindGroupLayout(goodDescriptor)
                
                val goodLayoutDesc = GPUPipelineLayoutDescriptor(
                    bindGroupLayouts = arrayOf(maxBgl, goodBgl)
                )
                doCreatePipelineLayoutTest(true, goodLayoutDesc)
                
                // Bad descriptor adds one more dynamic offset, overflowing pipeline limits
                val badDescriptor = GPUBindGroupLayoutDescriptor(
                    entries = arrayOf(
                        GPUBindGroupLayoutEntry(
                            binding = 0,
                            visibility = visibility,
                            buffer = GPUBufferBindingLayout(
                                type = type,
                                hasDynamicOffset = true
                            )
                        )
                    )
                )
                val badBgl = device.createBindGroupLayout(badDescriptor)
                
                val badLayoutDesc = GPUPipelineLayoutDescriptor(
                    bindGroupLayouts = arrayOf(maxBgl, badBgl)
                )
                doCreatePipelineLayoutTest(false, badLayoutDesc)
            }
        }
    }

    @Test
    fun testNumberOfBindGroupLayoutsExceedsTheMaximumValue(): Unit = runBlocking {
        val maxBindGroups = device.getLimits().maxBindGroups
        
        val emptyBglDesc = GPUBindGroupLayoutDescriptor(entries = emptyArray())
        val bindGroupLayouts = mutableListOf<GPUBindGroupLayout>()
        for (i in 0 until maxBindGroups) {
            bindGroupLayouts.add(device.createBindGroupLayout(emptyBglDesc))
        }
        
        val goodDesc = GPUPipelineLayoutDescriptor(
            bindGroupLayouts = bindGroupLayouts.toTypedArray()
        )
        doCreatePipelineLayoutTest(true, goodDesc)
        
        // Add one more
        bindGroupLayouts.add(device.createBindGroupLayout(emptyBglDesc))
        val badDesc = GPUPipelineLayoutDescriptor(
            bindGroupLayouts = bindGroupLayouts.toTypedArray()
        )
        doCreatePipelineLayoutTest(false, badDesc)
    }
}
