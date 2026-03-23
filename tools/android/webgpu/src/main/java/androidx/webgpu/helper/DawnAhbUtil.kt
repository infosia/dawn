package androidx.webgpu.helper

import android.hardware.HardwareBuffer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.webgpu.GPUDevice
import androidx.webgpu.GPUExternalTextureDescriptor

public object DawnAhbUtil {

  @RequiresApi(Build.VERSION_CODES.Q)
  public fun importRgbAhb(device: GPUDevice, ahb: HardwareBuffer): GPUImportedRgbAhbTexture {
    return importRgbAhbNative(device, ahb)
      ?: throw IllegalStateException("Dawn failed to import RGB HardwareBuffer")
  }

  @JvmStatic
  private external fun importRgbAhbNative(
    device: GPUDevice,
    ahb: HardwareBuffer,
  ): GPUImportedRgbAhbTexture?

  @RequiresApi(Build.VERSION_CODES.Q)
  public fun importYuvAhb(
    device: GPUDevice,
    ahb: HardwareBuffer,
    descriptor: GPUExternalTextureDescriptor,
  ): GPUImportedYuvAhbTexture {
    return importYuvAhbNative(device, ahb, descriptor)
      ?: throw IllegalStateException("Dawn failed to import YUV HardwareBuffer")
  }

  @JvmStatic
  private external fun importYuvAhbNative(
    device: GPUDevice,
    ahb: HardwareBuffer,
    descriptor: GPUExternalTextureDescriptor,
  ): GPUImportedYuvAhbTexture?
}