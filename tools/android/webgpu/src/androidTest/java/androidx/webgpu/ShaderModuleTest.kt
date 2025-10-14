package androidx.webgpu

import androidx.test.filters.SmallTest
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import junit.framework.TestCase.assertTrue
import kotlinx.coroutines.runBlocking
import org.junit.Test
import junit.framework.TestCase.assertEquals
import org.junit.After
import org.junit.Before

@SmallTest
class ShaderModuleTest {
  private lateinit var webGpu: WebGpu
  private lateinit var device: Device

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

  // The info request should still succeed even if compilation fails.
  private suspend fun getCompilationInfo(code: String): CompilationInfo {
    val shaderModule =
      device.createShaderModule(ShaderModuleDescriptor(shaderSourceWGSL = ShaderSourceWGSL(code)))
    val (_, info) = shaderModule.getCompilationInfo()
    return info
  }


  @Test
  fun getCompilationInfoReturnsNoErrorsForValidAsciiShader() {
    val code = """
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return vec4<f32>(0.0, 0.0, 0.0, 1.0);
            }
        """.trimIndent()
    val info = runBlocking { getCompilationInfo(code) }
    val errorCount = info.messages.count { it.type == CompilationMessageType.Error }
    assertEquals(0, errorCount)
  }

  @Test
  fun getCompilationInfoReturnsErrorsForInvalidAsciiShader() {
    val code = """
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return unknown(0.0, 0.0, 0.0, 1.0);
            }
        """.trimIndent()
    device.pushErrorScope(ErrorFilter.Validation)
    val info = runBlocking { getCompilationInfo(code) }
    val errorCount = info.messages.count { it.type == CompilationMessageType.Error }
    assertEquals(1, errorCount)
  }

  @Test
  fun lineNumberAndPositionAreCorrectForInvalidAsciiShader() {
    val expectedErrorLine = 2
    val code = """
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return unknown(0.0, 0.0, 0.0, 1.0);
            }
        """.trimIndent()
    device.pushErrorScope(ErrorFilter.Validation)

    val info = runBlocking { getCompilationInfo(code) }

    val foundAppropriateError = info.messages.any { message ->
      if (message.type != CompilationMessageType.Error) return@any false
      // An unknown location (0, 0) is acceptable
      if (message.lineNum == 0L) return@any true
      // Check if the error is on the expected line
      message.lineNum == expectedErrorLine.toLong()
    }
    assertTrue(
      "Expected an error on line $expectedErrorLine or at an unknown location.",
      foundAppropriateError
    )
  }

  @Test
  fun offsetAndLengthAreValidForAsciiShader() {
    val code = """
        @vertex fn main() -> @builtin(position) vec4<f32> {
          // error on next line
          return unknown(0.0, 0.0, 0.0, 1.0);
        }
    """.trimIndent()

    device.pushErrorScope(ErrorFilter.Validation)
    val info = runBlocking { getCompilationInfo(code) }

    val errorScope = runBlocking { device.popErrorScope() }
    assertEquals(ErrorType.Validation, errorScope.type)

    for (message in info.messages) {
      assertTrue(
        "Message offset/length is out of bounds",
        message.offset <= code.length && message.offset + message.length <= code.length
      )
    }
  }
}