package com.webgpu.demo

import androidx.compose.foundation.AndroidExternalSurface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.key
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
import androidx.webgpu.GPURequestAdapterOptions
import androidx.webgpu.GPUSurfaceConfiguration
import androidx.webgpu.TextureFormat
import androidx.webgpu.TextureUsage
import androidx.webgpu.helper.WebGpu
import androidx.webgpu.helper.createWebGpu
import kotlinx.coroutines.launch
import androidx.webgpu.GPUDawnTogglesDescriptor
import androidx.webgpu.DeviceLostCallback
import androidx.webgpu.DeviceLostException
import androidx.webgpu.FeatureName
import androidx.webgpu.GPUDeviceDescriptor
import java.util.concurrent.Executor
import androidx.webgpu.UncapturedErrorCallback
import androidx.webgpu.ExperimentalWebGpuApi
import kotlin.OptIn


/**
 * Options for configuring the graphics service.
 * @param scale Value to reduce the size of the surface for performance. The default value is 1,
 * which means the surface has the same dimensions as the UI element. Using a value of 2 will
 * halve the surface's width and height, trading quality for performance.
 * @param usage The usage flags of the surface's underlying texture.
 * @param requestAdapterOptions Options for requesting the WebGPU adapter.
 */
data class Options(
  val scale: Int = 1,
  @TextureUsage.Type val usage: Int = TextureUsage.RenderAttachment,
  val requestAdapterOptions: GPURequestAdapterOptions = GPURequestAdapterOptions(),
)

@Composable
  /**
   * [GraphicsSurface] is a helper composable for WebGPU enabled apps under Compose. It creates the
   * WebGPU device, instance, and surface and manages the render loop.
   *
   * @param modifier The modifier to be applied to the layout.
   * @param options Configuration options for the graphics service.
   * @param makeRenderer A client-provided factory function that returns a [com.webgpu.demo.Renderer]. This is called
   * once WebGPU is initialized and the surface is ready to be drawn on.
   */
@OptIn(ExperimentalWebGpuApi::class)
fun GraphicsSurface(
  modifier: Modifier = Modifier,
  options: Options,
  @TextureUsage.Type usage: Int = TextureUsage.RenderAttachment,
  makeRenderer: (webGpu: WebGpu, width: Int, height: Int) -> Renderer,
) {

  key(options) {
    AndroidExternalSurface(modifier = modifier) {

      onSurface { surface, surfaceWidth, surfaceHeight ->
        val scale = options.scale
        val width = surfaceWidth / scale
        val height = surfaceHeight / scale
        val webgpu =
          createWebGpu(
            surface,
            requestAdapterOptions = options.requestAdapterOptions,
            deviceDescriptor = GPUDeviceDescriptor(
              deviceLostCallback = DeviceLostCallback { device, reason, message ->
                throw DeviceLostException(device, reason, message)
              },
              deviceLostCallbackExecutor = Executor(Runnable::run),
              uncapturedErrorCallback = UncapturedErrorCallback { _, type, message ->
                throw RuntimeException("Uncaptured error: $type, $message")
              },

              uncapturedErrorCallbackExecutor = Executor(Runnable::run),
              requiredFeatures = intArrayOf(
                FeatureName.SharedTextureMemoryAHardwareBuffer,
                FeatureName.SharedFenceSyncFD,
                FeatureName.YCbCrVulkanSamplers,
                FeatureName.OpaqueYCbCrAndroidForExternalTexture,
                FeatureName.OpaqueYCbCrAndroidForExternalTexture
              )
            ).apply {
              dawnTogglesDescriptor =
                GPUDawnTogglesDescriptor(enabledToggles = arrayOf("allow_unsafe_apis"))
            }
          )
        // Configure the WebGPU surface from the Android surface's dimensions and standard
        // format.
        webgpu.webgpuSurface.configure(
          GPUSurfaceConfiguration(
            usage = options.usage,
            device = webgpu.device,
            format = TextureFormat.RGBA8Unorm,
            width = width,
            height = height
          )
        )

        // Call the GraphicsSurface client and receive its render object.
        val renderer = makeRenderer(webgpu, width, height)

        // Used to ensure the renderer stops being called ASAP when the surface is destroyed.
        var closing = false

        surface.onDestroyed {
          closing = true
          renderer.close()  // Call the client's close method.
          // Close out the WebGPU helper that contains the instance, device and surface.
          webgpu.close()
        }

        // Start the render loop.
        suspend fun nextFrame() {
          withFrameNanos { frameTimeNanos ->
            launch {
              if (!closing) {
                // Call the client to do its rendering.
                renderer.render(frameTimeNanos)
                // We trigger a new frame render only when the last has completed setup.
                nextFrame()
              }
            }
          }
        }
        nextFrame()   // Bootstrap the rendering.
      }
    }
  }
}
