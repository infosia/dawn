package androidx.webgpu.helper
import android.hardware.HardwareBuffer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.webgpu.GPUDevice

public object DawnAhbUtil {

    @RequiresApi(Build.VERSION_CODES.Q)
    public fun importRgbAhb(device: GPUDevice, ahb: HardwareBuffer): GPUImportedRGBHardwareBuffer {
        return importRgbAhbNative(device, ahb)
    }

    private external fun importRgbAhbNative(device: GPUDevice, ahb: HardwareBuffer): GPUImportedRGBHardwareBuffer
}