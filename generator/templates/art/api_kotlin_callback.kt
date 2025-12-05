// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package {{ kotlin_package }}

import androidx.webgpu.ErrorType
import androidx.webgpu.Status
import androidx.webgpu.WebGpuException
import androidx.webgpu.getException
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlinx.coroutines.suspendCancellableCoroutine

/**
 * A generic callback interface for asynchronous GPU requests.
 * @param T The type of the successful result object.
 */
public interface GPURequestCallback<T> {
  /**
   * Called when the request successfully returns a result.
   * @param result The successfully retrieved object.
   */
  public fun onResult(result: T)

  /**
   * Called when the request fails.
   * @param status A status code indicating the type of error.
   * @param message A human-readable error message.
   */
  public fun onError(exception: Exception)
}


/**
 * Converts a GPU request that takes a GPURequestCallback into a suspending coroutine function.
 */
public suspend fun <T> awaitGPURequest(
  block: (callback: GPURequestCallback<T>) -> Unit,
): T = suspendCancellableCoroutine { continuation ->
  block(object : GPURequestCallback<T> {
    override fun onResult(result: T) {
      if (continuation.isActive) {
        continuation.resume(result)
      }
    }


    override fun onError(exception: Exception) {
      if (continuation.isActive) {
        continuation.resumeWithException(exception)
      }
    }
  })
}

internal class GPURequestCallbackRunnable<T>(
  private val callback: GPURequestCallback<T>,
  private val status: Int,
  private val message: String,
  private val result: T?,
) : Runnable {
  override fun run() {
    if (status != Status.Success) {
      callback.onError(WebGpuException(status = status, reason = message))
    } else if (result == null) {
      callback.onError(WebGpuException(status = status, reason = "Null value returned"))
    } else {
      callback.onResult(result)
    }
  }
}

internal class GPURequestCallbackErrorTypeRunnable(
  private val callback: GPURequestCallback<@ErrorType Int>,
  private val status: Int,
  private val result: @ErrorType Int,
  private val message: String,
) : Runnable {
  override fun run() {
    if (status != Status.Success) {
      callback.onError(WebGpuException(status = status, reason = message))
    } else if (result != ErrorType.NoError) {
      callback.onError(getException(result, message))
    } else {
      callback.onResult(result)
    }
  }
}