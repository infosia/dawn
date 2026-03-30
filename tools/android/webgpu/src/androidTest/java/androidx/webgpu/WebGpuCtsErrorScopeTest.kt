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
class WebGpuCtsErrorScopeTest {
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

    private fun generateValidationError() {
        val descriptor = GPUShaderModuleDescriptor(
            shaderSourceWGSL = GPUShaderSourceWGSL("invalid code") // This trips validation
        )
        device.createShaderModule(descriptor)
    }

    private fun generateOomError() {
        val descriptor = GPUBufferDescriptor(
            size = 0xFFFFFFFFFFF00L, // Impossible memory bound trips OOM
            usage = BufferUsage.Storage
        )
        device.createBuffer(descriptor)
    }

    private fun triggerError(filter: Int) {
        when (filter) {
            ErrorFilter.Validation -> generateValidationError()
            ErrorFilter.OutOfMemory -> generateOomError()
        }
    }

    private suspend fun drainErrorScope(): Exception? {
        try {
            device.popErrorScope()
            return null
        } catch (e: WebGpuRuntimeException) {
            return e
        } catch (e: Exception) {
            return e
        }
    }

    @Test
    fun testSimple() = runBlocking {
        val filters = listOf(ErrorFilter.Validation)
        for (errorType in filters) {
            for (errorFilter in filters) {
                device.pushErrorScope(errorFilter)

                var syncThrownException: Exception? = null
                try {
                    triggerError(errorType)
                } catch (e: Exception) {
                    syncThrownException = e
                }

                var poppedExceptionClass: Class<out Exception>? = null
                val poppedErrorValue = try {
                    device.popErrorScope()
                } catch (e: Exception) {
                    poppedExceptionClass = e.javaClass
                    0
                }

                val expectedExceptionClass = if (errorType == ErrorFilter.Validation) {
                    ValidationException::class.java
                } else {
                    OutOfMemoryException::class.java
                }

                if (errorType == errorFilter) {
                    assert(syncThrownException?.javaClass == expectedExceptionClass || poppedExceptionClass == expectedExceptionClass) {
                        "Expected error to be thrown synchronously ($expectedExceptionClass) or popped ($expectedExceptionClass), but got sync: ${syncThrownException?.javaClass}, popped: $poppedExceptionClass"
                    }
                } else {
                    // if it wasn't captured, it must have been thrown synchronously due to default handler!
                    // So we expect poppedErrorValue == ErrorType.NoError (1) or some exception if it failed popping
                    if (poppedExceptionClass == null) {
                        assert(poppedErrorValue == ErrorType.NoError) {
                            "Expected NoError (1) to be popped, but got $poppedErrorValue"
                        }
                    } else {
                        // In Android, because it always throws synchronously regardless of filter,
                        // this test actually acts purely as a routing check. 
                        // But if it popped an exception anyway, we fail because it shouldn't have been captured!
                        assert(false) { "Expected no error to be successfully captured by mismatched filter" }
                    }
                }
            }
        }
    }

    @Test
    fun testEmpty() = runBlocking {
        var rejected = false
        try {
            device.popErrorScope()
        } catch (e: Exception) {
            rejected = true
        }
        assert(rejected) { "Popping empty error scope should reject/throw" }
    }

    @Test
    fun testBalancedSiblings() = runBlocking {
        val numErrors = listOf(1, 10, 100)
        for (n in numErrors) {
            for (i in 0 until n) {
                device.pushErrorScope(ErrorFilter.Validation)
                device.popErrorScope() // Success returns NoError, so no exception
            }

            var rejected = false
            try {
                device.popErrorScope()
            } catch (e: Exception) {
                rejected = true
            }
            assert(rejected) { "Popping unpushed scope should reject" }
        }
    }

    @Test
    fun testBalancedNesting() = runBlocking {
        val numErrors = listOf(1, 10, 100)
        for (n in numErrors) {
            for (i in 0 until n) {
                device.pushErrorScope(ErrorFilter.Validation)
            }
            for (i in 0 until n) {
                device.popErrorScope()
            }

            var rejected = false
            try {
                device.popErrorScope()
            } catch (e: Exception) {
                rejected = true
            }
            assert(rejected) { "Popping unpushed scope should reject" }
        }
    }
}
