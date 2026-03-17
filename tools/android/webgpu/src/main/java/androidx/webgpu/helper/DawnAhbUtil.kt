package androidx.webgpu.helper
import android.hardware.HardwareBuffer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.webgpu.GPUDevice
import androidx.webgpu.GPUExternalTextureDescriptor

public object DawnAhbUtil {

    @RequiresApi(Build.VERSION_CODES.Q)
    public fun importRgbAhb(device: GPUDevice, ahb: HardwareBuffer): GPUImportedRGBHardwareBuffer {
        return importRgbAhbNative(device, ahb)
    }

    private external fun importRgbAhbNative(device: GPUDevice, ahb: HardwareBuffer): GPUImportedRGBHardwareBuffer

    public fun importYuvAhb(
        device: GPUDevice,
        ahb: HardwareBuffer,
        descriptor: GPUExternalTextureDescriptor
    ): GPUImportedYuvHardwareBuffer {

        val result = importYuvAhbNative(device, ahb, descriptor)
            ?: throw IllegalStateException("Dawn failed to import YUV HardwareBuffer")

        return result
    }

    @JvmStatic
    private external fun importYuvAhbNative(
        device: GPUDevice,
        ahb: HardwareBuffer,
        descriptor: GPUExternalTextureDescriptor
    ): GPUImportedYuvHardwareBuffer?
}