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

  companion object {
    private const val INVALID_SHADER = """
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return unknown(0.0, 0.0, 0.0, 1.0);
            }
        """
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
    val code = INVALID_SHADER.trimIndent()
    device.pushErrorScope(ErrorFilter.Validation)
    val info = runBlocking { getCompilationInfo(code) }
    val errorCount = info.messages.count { it.type == CompilationMessageType.Error }
    assertEquals(1, errorCount)
  }

  /**
   * Verifies that for an invalid shader, the reported error location is correct and consistent.
   *
   * This test checks three things for each compilation error message:
   * 1. The line number (`lineNum`) points to the correct line in the source code.
   * 2. The absolute character `offset` and `length` are within the bounds of the source code string.
   * 3. The `offset` is consistent with the `lineNum` and `linePos` (line position).
   * This is verified by calculating the offset from the line/position and ensuring it matches.
   */
  @Test
  fun compilationErrorLocationIsCorrectForInvalidShader() {
    val code = INVALID_SHADER.trimIndent()
    val expectedErrorLine = 2L // The line number from the compiler is 1-based.
    val lines = code.split('\n')

    device.pushErrorScope(ErrorFilter.Validation)
    val info = runBlocking { getCompilationInfo(code) }
    val errorScope = runBlocking { device.popErrorScope() }
    assertEquals(ErrorType.Validation, errorScope?.type)

    val errorMessages = info.messages.filter { it.type == CompilationMessageType.Error }
    assertTrue("Expected at least one compilation error message.", errorMessages.isNotEmpty())

    for (message in errorMessages) {
      // Check that the line number is correct (or 0 for an unknown location).
      assertTrue(
        "Expected error on line $expectedErrorLine or at an unknown location (line 0).",
        message.lineNum == expectedErrorLine || message.lineNum == 0L
      )

      // Check that offset and length are within the string bounds.
      assertTrue(
        "Message offset (${message.offset}) and length (${message.length}) are out of bounds for code length ${code.length}.",
        message.offset <= code.length && message.offset + message.length <= code.length
      )

      // Verify that the offset can be derived from lineNum and linePos.
      // This consistency check can only be done if the location is known (lineNum > 0).
      if (message.lineNum > 0L) {
        // `lineNum` is 1-based, so we convert to a 0-based index.
        val lineIndex = (message.lineNum - 1).toInt()

        // Calculate the offset: sum lengths of preceding lines (+1 for each newline)
        // and add the 0-based character position on the error line.
        val calculatedOffset = lines.take(lineIndex).sumOf { it.length + 1 } + (message.linePos - 1) // Corrected line

        assertEquals(
          "Calculated offset from line/pos should match message offset.",
          message.offset, // The expected value comes from the compiler
          calculatedOffset // The actual value is what we calculated
        )
      }
    }
  }
}