package android.dawn.helper

import android.dawn.CallbackMode.Companion.WaitAnyOnly
import android.dawn.Device
import android.dawn.DeviceDescriptor
import android.dawn.DeviceLostCallback
import android.dawn.DeviceLostCallbackInfo
import android.dawn.DeviceLostReason
import android.dawn.ErrorType
import android.dawn.FeatureName
import android.dawn.Instance
import android.dawn.InstanceDescriptor
import android.dawn.RequestAdapterOptions
import android.dawn.RequestAdapterStatus
import android.dawn.RequestDeviceStatus
import android.dawn.SurfaceDescriptor
import android.dawn.SurfaceSourceAndroidNativeWindow
import android.dawn.UncapturedErrorCallbackInfo
import android.dawn.createInstance
import android.dawn.helper.Util.windowFromSurface
import android.dawn.requestAdapter
import android.dawn.requestDevice
import android.os.Handler
import android.os.Looper
import android.view.Surface

public class DeviceLostException(
    public val device: Device, public val reason: DeviceLostReason, message: String
) : Exception(message)

public class UncapturedErrorException(
    public val device: Device, public val type: ErrorType, message: String
) : Exception(message)

private const val POLLING_DELAY_MS = 100L

public abstract class WebGpu : AutoCloseable {
    public abstract val instance: Instance
    public abstract val webgpuSurface: android.dawn.Surface?
    public abstract val device: Device
}

public suspend fun getWebGpu(
    surface: Surface? = null,
    instanceDescriptor: InstanceDescriptor = InstanceDescriptor(),
    requestAdapterOptions: RequestAdapterOptions = RequestAdapterOptions(),
    requiredFeatures: Array<FeatureName> = arrayOf()
): WebGpu {
    initLibrary()

    val instance = createInstance(instanceDescriptor)
    val webgpuSurface = if (surface == null) null else instance.createSurface(
        SurfaceDescriptor(
            surfaceSourceAndroidNativeWindow = SurfaceSourceAndroidNativeWindow(
                windowFromSurface(surface)
            )
        )
    )
    val (status, adapter, message) = instance.requestAdapter(requestAdapterOptions)
    check(status == RequestAdapterStatus.Success && adapter != null) {
        message.ifEmpty { "Error requesting the adapter: $status" }
    }

    val (status1, device, message1) = adapter.requestDevice(
        DeviceDescriptor(
            requiredFeatures = requiredFeatures,
            deviceLostCallbackInfo = DeviceLostCallbackInfo(
                callback = DeviceLostCallback { device, reason, message ->
                    throw DeviceLostException(device, reason, message)
                }, mode = WaitAnyOnly
            ),
            uncapturedErrorCallbackInfo = UncapturedErrorCallbackInfo { device, type, message ->
                throw UncapturedErrorException(device, type, message)
            })
    )
    check(status1 == RequestDeviceStatus.Success && device != null) {
        message1.ifEmpty { "Error requesting the device: $status" }
    }

    val isClosing = false
    // Long-running event poller for async methods. Can be removed when
    // https://issues.chromium.org/issues/323983633 is fixed.
    val handler = Handler(Looper.getMainLooper())
    fun nextProcess() {
        handler.postDelayed({
            if (isClosing) {
                return@postDelayed
            }
            instance.processEvents()
            nextProcess()
        }, POLLING_DELAY_MS)
    }
    nextProcess()

    return object : WebGpu() {
        override val instance = instance
        override val webgpuSurface = webgpuSurface
        override val device = device!!

        override fun close() {
            device.close()
            device.destroy();
//          webgpuSurface.close()  // TODO(b/428866400): Remove when fixed
//          instance.close()  // TODO(b/428866400): Remove when fixed
        }
    }
}

/**
 * Initializes the native library. This method should be called before making and WebGPU calls.
 */
public fun initLibrary() {
    System.loadLibrary("webgpu_c_bundled")
}
