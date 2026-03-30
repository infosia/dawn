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

class WebGpuCtsComputePipelineTest {
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

    private fun getShaderModule(
        shaderStage: String = "compute",
        entryPoint: String = "main"
    ): GPUShaderModule {
        val code = when (shaderStage) {
            "compute" -> "@compute @workgroup_size(1) fn $entryPoint() {}"
            "vertex" -> "@vertex fn $entryPoint() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0, 0.0, 0.0, 1.0); }"
            "fragment" -> "@fragment fn $entryPoint() {}"
            else -> "@compute @workgroup_size(1) fn $entryPoint() {}"
        }
        return device.createShaderModule(
            GPUShaderModuleDescriptor(
                shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
            )
        )
    }

    private fun doCreateComputePipelineTest(
        success: Boolean,
        descriptor: GPUComputePipelineDescriptor
    ) = runBlocking {
        device.pushErrorScope(ErrorFilter.Validation)
        var errorThrown = false
        try {
            device.createComputePipeline(descriptor)
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
    fun testBasic(): Unit = runBlocking {
        val descriptor = GPUComputePipelineDescriptor(
            layout = null,
            compute = GPUComputeState(
                module = getShaderModule("compute", "main"),
                entryPoint = "main"
            )
        )
        doCreateComputePipelineTest(true, descriptor)
    }

    @Test
    fun testShaderModuleInvalid(): Unit = runBlocking {
        try {
            val invalidModule = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = "invalid wgsl code")
                )
            )
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = invalidModule,
                    entryPoint = "main"
                )
            )
            doCreateComputePipelineTest(false, descriptor)
        } catch (e: androidx.webgpu.ValidationException) {
            // Success: Dawn caught the WGSL parsing error synchronously
            assert(true)
        }
    }

    @Test
    fun testShaderModuleCompute(): Unit = runBlocking {
        val stages = listOf("compute", "vertex", "fragment")
        for (stage in stages) {
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = getShaderModule(stage, "main"),
                    entryPoint = "main"
                )
            )
            val success = stage == "compute"
            doCreateComputePipelineTest(success, descriptor)
        }
    }

    @Test
    fun testLimitsWorkgroupStorageSize(): Unit = runBlocking {
        val limits = device.getLimits()
        val maxWorkgroupStorageSize = limits.maxComputeWorkgroupStorageSize
        
        class TestCase(val type: String, val typeSize: Int)
        val cases = listOf(
            TestCase("vec4<f32>", 16),
            TestCase("mat4x4<f32>", 64)
        )
        
        for (case in cases) {
            for (countDeltaFromLimit in listOf(0, 1)) {
                val countAtLimit = maxWorkgroupStorageSize / case.typeSize
                val count = countAtLimit + countDeltaFromLimit
                
                val code = """
                    var<workgroup> data: array<${case.type}, $count>;
                    @compute @workgroup_size(64) fn main() {
                        _ = data;
                    }
                """.trimIndent()
                
                val module = device.createShaderModule(
                    GPUShaderModuleDescriptor(
                        shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                    )
                )
                val descriptor = GPUComputePipelineDescriptor(
                    layout = null,
                    compute = GPUComputeState(
                        module = module,
                        entryPoint = "main"
                    )
                )
                val success = count <= countAtLimit
                doCreateComputePipelineTest(success, descriptor)
            }
        }
    }

    @Test
    fun testLimitsInvocationsPerWorkgroup(): Unit = runBlocking {
        val limits = device.getLimits()
        val sizes = listOf(
            listOf(128, 1, 2),
            listOf(129, 1, 2),
            listOf(2, 128, 1),
            listOf(2, 129, 1),
            listOf(1, 8, 32),
            listOf(1, 8, 33)
        )
        
        for (size in sizes) {
            val code = """
                @compute @workgroup_size(${size.joinToString(",")}) fn main() {
                }
            """.trimIndent()
            
            val module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            )
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = module,
                    entryPoint = "main"
                )
            )
            val success = size[0] * size[1] * size[2] <= limits.maxComputeInvocationsPerWorkgroup
            doCreateComputePipelineTest(success, descriptor)
        }
    }
    
    @Test
    fun testLimitsInvocationsPerWorkgroupEachComponent(): Unit = runBlocking {
        val limits = device.getLimits()
        val sizes = listOf(
            listOf(64, 1, 1),
            listOf(256, 1, 1),
            listOf(257, 1, 1),
            listOf(1, 256, 1),
            listOf(1, 257, 1),
            listOf(1, 1, 63),
            listOf(1, 1, 64),
            listOf(1, 1, 65)
        )
        
        for (size in sizes) {
            val code = """
                @compute @workgroup_size(${size.joinToString(",")}) fn main() {
                }
            """.trimIndent()
            
            val module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            )
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = module,
                    entryPoint = "main"
                )
            )
            val success = size[0] <= limits.maxComputeWorkgroupSizeX && 
                          size[1] <= limits.maxComputeWorkgroupSizeY && 
                          size[2] <= limits.maxComputeWorkgroupSizeZ
            doCreateComputePipelineTest(success, descriptor)
        }
    }
    
    @Test
    fun testOverridesIdentifier(): Unit = runBlocking {
        class TestCase(val constants: Map<String, Double>, val success: Boolean)
        val cases = listOf(
            TestCase(emptyMap(), true),
            TestCase(mapOf("c0" to 0.0), true),
            TestCase(mapOf("c0" to 0.0, "c1" to 1.0), true),
            // Kotlin/Java string handles unicode naturally, but certain ones WGSL expects cleanly
            TestCase(mapOf("c9" to 0.0), false),
            TestCase(mapOf("1" to 0.0), true),
            TestCase(mapOf("c3" to 0.0), false),
            TestCase(mapOf("2" to 0.0), false),
            TestCase(mapOf("1000" to 0.0), true),
            TestCase(mapOf("9999" to 0.0), false),
            TestCase(mapOf("1000" to 0.0, "c2" to 0.0), false)
        )
        for (case in cases) {
            val code = """
                override c0: bool = true;
                override c1: u32 = 0u;
                @id(1000) override c2: u32 = 10u;
                @id(1) override c3: u32 = 11u;
                @compute @workgroup_size(1) fn main() {
                    _ = u32(c0);
                    _ = u32(c1);
                    _ = u32(c2);
                    _ = u32(c3);
                }
            """.trimIndent()
            val module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            )
            
            val constantEntries = case.constants.map { 
                GPUConstantEntry(key = it.key, value = it.value) 
            }.toTypedArray()
            
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = module,
                    entryPoint = "main",
                    constants = constantEntries
                )
            )
            doCreateComputePipelineTest(case.success, descriptor)
        }
    }
    
    @Test
    fun testOverridesUninitialized(): Unit = runBlocking {
        class TestCase(val constants: Map<String, Double>, val success: Boolean)
        val cases = listOf(
            TestCase(emptyMap(), false),
            TestCase(mapOf("c0" to 0.0, "c2" to 0.0, "c8" to 0.0), false),
            TestCase(mapOf("c0" to 0.0, "c2" to 0.0, "c5" to 0.0, "c8" to 0.0), true),
            TestCase(mapOf("c0" to 0.0, "c2" to 0.0, "c5" to 0.0, "c8" to 0.0, "c1" to 0.0), true)
        )
        for (case in cases) {
            val code = """
                override c0: bool;
                override c1: bool = false;
                override c2: f32;
                override c3: f32 = 0.0;
                override c4: f32 = 4.0;
                override c5: i32;
                override c6: i32 = 0;
                override c7: i32 = 7;
                override c8: u32;
                override c9: u32 = 0u;
                @id(1000) override c10: u32 = 10u;
                @compute @workgroup_size(1) fn main() {
                    _ = u32(c0);
                    _ = u32(c1);
                    _ = u32(c2);
                    _ = u32(c3);
                    _ = u32(c4);
                    _ = u32(c5);
                    _ = u32(c6);
                    _ = u32(c7);
                    _ = u32(c8);
                    _ = u32(c9);
                    _ = u32(c10);
                }
            """.trimIndent()
            
            val module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = code)
                )
            )
            
            val constantEntries = case.constants.map { 
                GPUConstantEntry(key = it.key, value = it.value) 
            }.toTypedArray()
            
            val descriptor = GPUComputePipelineDescriptor(
                layout = null,
                compute = GPUComputeState(
                    module = module,
                    entryPoint = "main",
                    constants = constantEntries
                )
            )
            doCreateComputePipelineTest(case.success, descriptor)
        }
    }
}
