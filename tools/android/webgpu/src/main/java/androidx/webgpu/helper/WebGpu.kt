/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
@file:JvmName("WebGpuUtils")

package androidx.webgpu.helper

import android.view.Surface
import androidx.webgpu.GPUAdapter
import androidx.webgpu.BackendType
import androidx.webgpu.GPUDevice
import androidx.webgpu.GPUDeviceDescriptor
import androidx.webgpu.DeviceLostCallback
import androidx.webgpu.DeviceLostException
import androidx.webgpu.GPUInstance
import androidx.webgpu.GPUInstanceDescriptor
import androidx.webgpu.GPURequestAdapterOptions
import androidx.webgpu.GPUSurface
import androidx.webgpu.GPUSurfaceDescriptor
import androidx.webgpu.GPUSurfaceSourceAndroidNativeWindow
import androidx.webgpu.UncapturedErrorCallback
import androidx.webgpu.GPU.createInstance
import androidx.webgpu.WebGpuRuntimeException
import androidx.webgpu.helper.Util.windowFromSurface
import java.util.concurrent.Executor
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import kotlinx.coroutines.currentCoroutineContext
import java.util.concurrent.Executors
import kotlinx.coroutines.ExecutorCoroutineDispatcher
import kotlin.jvm.Volatile

private val activeDispatcher = ThreadLocal<CoroutineDispatcher?>()

/**
 * Single-threaded [CoroutineDispatcher] dedicated to running all WebGPU/Dawn JNI operations.
 *
 * The native Dawn engine is not thread-safe for concurrent access. To prevent race conditions
 * and segmentation faults, all WebGPU operations (including initialization, processEvents polling,
 * queue submission, and teardown/cleanup) must be executed sequentially on this dispatcher thread.
 */
private val defaultWebGpuDispatcher: CoroutineDispatcher
    get() {
        val holder = object {
            @Volatile var dispatcher: CoroutineDispatcher? = null
        }
        val dispatcher = Executors.newSingleThreadExecutor { runnable ->
            Thread({
                activeDispatcher.set(holder.dispatcher)
                runnable.run()
            }, "WebGPU-Thread").apply { isDaemon = true }
        }.asCoroutineDispatcher()
        holder.dispatcher = dispatcher
        return dispatcher
    }

private const val POLLING_DELAY_MS = 16L // 60 FPS polling

public abstract class WebGpu : AutoCloseable {
    public abstract val instance: GPUInstance
    public abstract val adapter: GPUAdapter
    public abstract val device: GPUDevice
    public abstract val webgpuSurface: GPUSurface
    public abstract val dispatcher: CoroutineDispatcher

    public suspend fun <T> execute(block: suspend () -> T): T {
        return withContext(dispatcher) {
            val original = activeDispatcher.get()
            activeDispatcher.set(dispatcher)
            try {
                block()
            } finally {
                activeDispatcher.set(original)
            }
        }
    }

    public suspend fun processEventsLoop() {
        withContext(dispatcher) {
            val original = activeDispatcher.get()
            activeDispatcher.set(dispatcher)
            try {
                while (currentCoroutineContext().isActive) {
                    instance.processEvents()
                    delay(POLLING_DELAY_MS)
                }
            } finally {
                activeDispatcher.set(original)
            }
        }
    }

}

private val defaultDeviceDescriptor
    get() = GPUDeviceDescriptor(
        deviceLostCallback = defaultDeviceLostCallback,
        deviceLostCallbackExecutor = Executor(Runnable::run),
        uncapturedErrorCallback = defaultUncapturedErrorCallback,
        uncapturedErrorCallbackExecutor = Executor(Runnable::run)
    )

public suspend fun createWebGpu(
    surface: Surface? = null,
    instanceDescriptor: GPUInstanceDescriptor = GPUInstanceDescriptor(),
    requestAdapterOptions: GPURequestAdapterOptions = GPURequestAdapterOptions(),
    deviceDescriptor: GPUDeviceDescriptor = defaultDeviceDescriptor
): WebGpu = createWebGpuInternal(
    dispatcher = defaultWebGpuDispatcher,
    isDispatcherOwned = true,
    surface = surface,
    instanceDescriptor = instanceDescriptor,
    requestAdapterOptions = requestAdapterOptions,
    deviceDescriptor = deviceDescriptor
)

public suspend fun createWebGpu(
    dispatcher: CoroutineDispatcher,
    surface: Surface? = null,
    instanceDescriptor: GPUInstanceDescriptor = GPUInstanceDescriptor(),
    requestAdapterOptions: GPURequestAdapterOptions = GPURequestAdapterOptions(),
    deviceDescriptor: GPUDeviceDescriptor = defaultDeviceDescriptor
): WebGpu = createWebGpuInternal(
    dispatcher = dispatcher,
    isDispatcherOwned = false,
    surface = surface,
    instanceDescriptor = instanceDescriptor,
    requestAdapterOptions = requestAdapterOptions,
    deviceDescriptor = deviceDescriptor
)

private suspend fun createWebGpuInternal(
    dispatcher: CoroutineDispatcher,
    isDispatcherOwned: Boolean,
    surface: Surface?,
    instanceDescriptor: GPUInstanceDescriptor,
    requestAdapterOptions: GPURequestAdapterOptions,
    deviceDescriptor: GPUDeviceDescriptor,
): WebGpu {
    initLibrary()

    val instance = createInstance(instanceDescriptor)
    val webgpuSurface =
        surface?.let {
            instance.createSurface(
                GPUSurfaceDescriptor(
                    surfaceSourceAndroidNativeWindow =
                        GPUSurfaceSourceAndroidNativeWindow(windowFromSurface(it))
                )
            )
        }

    val adapter = requestAdapter(instance, requestAdapterOptions)
    val device = requestDevice(adapter, deviceDescriptor)

    return object : WebGpu() {
        override val instance = instance
        override val adapter = adapter
        override val webgpuSurface
            get() = checkNotNull(webgpuSurface)
        override val device = device
        override val dispatcher = dispatcher

        override fun close() {
            //device.close() // TODO(b/428866400): Uncomment when fixed.
            val cleanup = {
                webgpuSurface?.close()
                instance.close()
                adapter.close()
                if (isDispatcherOwned) {
                    (dispatcher as? ExecutorCoroutineDispatcher)?.close()
                }
            }
            if (activeDispatcher.get() == dispatcher) {
                cleanup()
            } else {
                runBlocking(dispatcher) {
                    cleanup()
                }
            }
        }
    }
}

private suspend fun requestAdapter(
    instance: GPUInstance,
    options: GPURequestAdapterOptions = GPURequestAdapterOptions(backendType = BackendType.Vulkan),
): GPUAdapter {
    return instance.requestAdapter(options)
}

private suspend inline fun requestDevice(
    adapter: GPUAdapter,
    deviceDescriptor: GPUDeviceDescriptor,
): GPUDevice {
    if (deviceDescriptor.deviceLostCallback == null) {
        deviceDescriptor.deviceLostCallback = defaultDeviceLostCallback
    }

    if (deviceDescriptor.uncapturedErrorCallback == null) {
        deviceDescriptor.uncapturedErrorCallback = defaultUncapturedErrorCallback
    }
    return adapter.requestDevice(deviceDescriptor)
}

private val defaultUncapturedErrorCallback
    get(): UncapturedErrorCallback {
        return UncapturedErrorCallback { _, type, message ->
            throw WebGpuRuntimeException.create(type, message)
        }
    }

private val defaultDeviceLostCallback get(): DeviceLostCallback {
    return DeviceLostCallback { device, reason, message ->
        throw DeviceLostException(device, reason, message)
    }
}

/** Initializes the native library. This method should be called before making and WebGPU calls. */
public fun initLibrary() {
    System.loadLibrary("webgpu_c_bundled")
}
