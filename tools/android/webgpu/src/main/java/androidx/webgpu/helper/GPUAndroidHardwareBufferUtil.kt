package androidx.webgpu.helper

import android.hardware.HardwareBuffer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.webgpu.GPUDevice
import androidx.webgpu.GPUExternalTextureDescriptor
import androidx.webgpu.GPUHardwareBufferExternalTexture
import androidx.webgpu.GPUHardwareBufferTexture

public object GPUAndroidHardwareBufferUtil {

  @RequiresApi(Build.VERSION_CODES.Q)
  public fun createTexture(device: GPUDevice, ahb: HardwareBuffer): GPUHardwareBufferTexture {
    return createTextureNative(device, ahb)
      ?: throw IllegalStateException("Dawn failed to import RGB HardwareBuffer")
  }

  @JvmStatic
  private external fun createTextureNative(
    device: GPUDevice,
    ahb: HardwareBuffer,
  ): GPUHardwareBufferTexture?

  @RequiresApi(Build.VERSION_CODES.Q)
  public fun createExternalTexture(
    device: GPUDevice,
    ahb: HardwareBuffer,
    descriptor: GPUExternalTextureDescriptor,
  ): GPUHardwareBufferExternalTexture {
    return createExternalTextureNative(device, ahb, descriptor)
      ?: throw IllegalStateException("Dawn failed to import YUV HardwareBuffer")
  }

  @JvmStatic
  private external fun createExternalTextureNative(
    device: GPUDevice,
    ahb: HardwareBuffer,
    descriptor: GPUExternalTextureDescriptor,
  ): GPUHardwareBufferExternalTexture?
}