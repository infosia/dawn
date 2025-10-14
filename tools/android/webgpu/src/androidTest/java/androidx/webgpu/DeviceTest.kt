package androidx.webgpu

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import androidx.webgpu.helper.DeviceLostException
import androidx.webgpu.helper.UncapturedErrorException
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import junit.framework.TestCase
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.Assert.assertThrows
import org.junit.Before

@RunWith(AndroidJUnit4::class)
@SmallTest
class DeviceTest {
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

  @Test
  @SmallTest
  fun testHasFeature() {
    runBlocking {
      // This test ensures the API is callable.
      device.hasFeature(FeatureName.TimestampQuery)
    }
  }

  @Test
  @SmallTest
  fun testErrorScope() {
    device.pushErrorScope(ErrorFilter.Validation)

    // Intentionally create an invalid buffer to trigger a validation error.
    // A buffer size must be a multiple of 4.
    device.createBuffer(
      BufferDescriptor(
        size = 1, usage = BufferUsage.Vertex, mappedAtCreation = true
      )
    )

    val error = runBlocking { device.popErrorScope() }
    TestCase.assertTrue(
      error.type == ErrorType.Validation && error.status == PopErrorScopeStatus.Success
    )
  }


  @Test
  @SmallTest
  fun testCreateBuffer() {
    val buffer = device.createBuffer(
      BufferDescriptor(
        size = 4, usage = BufferUsage.Vertex
      )
    )
    TestCase.assertEquals(buffer.usage, BufferUsage.Vertex)
  }


  @Test
  @SmallTest
  fun testCreateTexture() {
    val texture = device.createTexture(
      TextureDescriptor(
        size = Extent3D(1, 1, 1),
        format = TextureFormat.RGBA8Unorm,
        usage = TextureUsage.TextureBinding
      )
    )
    TestCase.assertEquals(texture.usage, TextureUsage.TextureBinding)
  }

  @Test
  @SmallTest
  fun testDestroy() {
    val webGpu = runBlocking { createWebGpu() }
    val device = webGpu.device
    assertThrows(DeviceLostException::class.java) {
      device.destroy()
    }
  }

  @Test
  @SmallTest
  fun testCreateComputePipeline_withInvalidEntryPoint_throwsException() {
    val shaderModule = device.createShaderModule(
      ShaderModuleDescriptor(
        shaderSourceWGSL = ShaderSourceWGSL(
          code = "@compute @workgroup_size(1) fn main() {}"
        )
      )
    )

    assertThrows(UncapturedErrorException::class.java) {
      device.createComputePipeline(
        ComputePipelineDescriptor(
          compute = ComputeState(
            module = shaderModule, entryPoint = "non_existent_entry_point"
          )
        )
      )
    }
  }

  @Test
  @SmallTest
  fun createShaderModule_withInvalidShader_throwsException() {
    // This shader has a syntax error ("fu" instead of "fn")
    val badShaderCode = "@compute @workgroup_size(1) fu main() {}"

    // Creating the shader module itself should fail
    assertThrows(Exception::class.java) {
      device.createShaderModule(
        ShaderModuleDescriptor(shaderSourceWGSL = ShaderSourceWGSL(code = badShaderCode))
      )
    }
  }

  /**
   * Verifies that createRenderPipeline fails validation if the entry point is incorrect.
   */
  @Test
  @SmallTest
  fun createRenderPipeline_withInvalidEntryPoint_failsValidation() {
    val shaderModule = device.createShaderModule(
      ShaderModuleDescriptor(
        shaderSourceWGSL = ShaderSourceWGSL(
          code = "@vertex fn main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0); }"
        )
      )
    )

    device.pushErrorScope(ErrorFilter.Validation)
    device.createRenderPipeline(
      RenderPipelineDescriptor(
        vertex = VertexState(
          module = shaderModule, entryPoint = "non_existent_entry_point" // Invalid
        )
      )
    )
    val error = runBlocking { device.popErrorScope() }
    TestCase.assertTrue(
      "Expected a validation error for a non-existent shader entry point",
      error.status == PopErrorScopeStatus.Success && error.type == ErrorType.Validation
    )
  }

  @Test
  @SmallTest
  fun createBindGroupLayout_withDuplicateBindings_failsValidation() {
    device.pushErrorScope(ErrorFilter.Validation)
    device.createBindGroupLayout(
      BindGroupLayoutDescriptor(
        entries = arrayOf(
          BindGroupLayoutEntry(
            binding = 0, // Duplicate
            visibility = ShaderStage.Fragment,
            buffer = BufferBindingLayout(type = BufferBindingType.Storage)
          ), BindGroupLayoutEntry(
            binding = 0, // Duplicate
            visibility = ShaderStage.Fragment,
            buffer = BufferBindingLayout(type = BufferBindingType.Storage)
          )
        )
      )
    )
    val error = runBlocking { device.popErrorScope() }

    TestCase.assertTrue(
      "Expected a validation error for duplicate binding numbers",
      error.status == PopErrorScopeStatus.Success && error.type == ErrorType.Validation
    )
  }

  /**
   * Verifies that createBindGroup fails validation if the buffer's usage is incorrect.
   */
  @Test
  @SmallTest
  fun createBindGroup_withMismatchedBufferUsage_failsValidation() {
    val layout = device.createBindGroupLayout(
      BindGroupLayoutDescriptor(
        entries = arrayOf(
          BindGroupLayoutEntry(
            binding = 0,
            visibility = ShaderStage.Compute,
            buffer = BufferBindingLayout(type = BufferBindingType.Uniform)
          )
        )
      )
    )

    // Create a buffer WITHOUT the required `Uniform` usage.
    val buffer = device.createBuffer(
      BufferDescriptor(size = 16, usage = BufferUsage.CopySrc) // Invalid usage
    )

    device.pushErrorScope(ErrorFilter.Validation)
    device.createBindGroup(
      BindGroupDescriptor(
        layout = layout, entries = arrayOf(
          BindGroupEntry(binding = 0, buffer = buffer)
        )
      )
    )
    val error = runBlocking { device.popErrorScope() }
    TestCase.assertTrue(
      "Expected a validation error for mismatched buffer usage",
      error.status == PopErrorScopeStatus.Success && error.type == ErrorType.Validation
    )
  }

  @Test
  @SmallTest
  fun createQuerySet_withInvalidCount_failsValidation() {
    device.pushErrorScope(ErrorFilter.Validation)
    device.createQuerySet(
      QuerySetDescriptor(
        type = QueryType.Occlusion, count = -1 // Invalid: count must be > 0.
      )
    )
    val error = runBlocking { device.popErrorScope() }
    assert(
      error.status == PopErrorScopeStatus.Success && error.type == ErrorType.Validation
    ) {
      "Expected a validation error for a query count of 0"
    }
  }

  /**
   * Verifies that createSampler fails validation with an invalid descriptor.
   */
  @Test
  @SmallTest
  fun createSampler_withInvalidLodClamp_failsValidation() = runBlocking {
    val invalidDescriptor = SamplerDescriptor(
      lodMinClamp = 10.0f, lodMaxClamp = 1.0f // Invalid: min cannot be greater than max.
    )

    device.pushErrorScope(ErrorFilter.Validation)
    device.createSampler(invalidDescriptor)
    val error = device.popErrorScope()

    assert(error.status == PopErrorScopeStatus.Success && error.type == ErrorType.Validation) {
      "Expected a validation error when lodMinClamp > lodMaxClamp"
    }
  }
}
