package com.dawn.ahbsupport

import android.graphics.Bitmap
import android.graphics.ColorSpace
import android.graphics.HardwareBufferRenderer
import android.graphics.RenderNode
import android.hardware.HardwareBuffer
import android.hardware.SyncFence
import android.media.Image
import android.media.ImageReader
import android.media.MediaPlayer
import androidx.annotation.RequiresApi
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext

import androidx.webgpu.*
import androidx.webgpu.helper.DawnAhbUtil
import androidx.webgpu.helper.GPUImportedRGBHardwareBuffer
import androidx.webgpu.helper.asString
import kotlinx.coroutines.suspendCancellableCoroutine
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference
import kotlin.coroutines.resume

//https://source.corp.google.com/h/dawn-android-samples/dawn-demo/+/main:app/src/main/java/com/android/dawndemo/CameraInputRenderer.kt
@Composable
fun ZeroCopyRGBVideo(modifier: Modifier, options: Options) {
  val context = LocalContext.current
  val assets = context.assets

  val videoWidth = 1280
  val videoHeight = 720

  // --- 1. SETUP THE VIDEO PLAYER & IMAGE READER ---
  val useImage = remember { AtomicReference<Image?>(null) }

  val imageReader = remember {
    ImageReader.newInstance(
      videoWidth, videoHeight,
      android.graphics.ImageFormat.PRIVATE,
      4,
      HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE
    ).apply {
      setOnImageAvailableListener({ reader ->
                                    try {
                                      val image = reader.acquireLatestImage()
                                      if (image != null) {
                                        val oldImage = useImage.getAndSet(image)
                                        oldImage?.close()
                                      }
                                    } catch (e: IllegalStateException) {
                                      // Decoder is running too fast, safely drop the frame
                                    }
                                  }, null)
    }
  }

  val mediaPlayer = remember {
    MediaPlayer.create(context, R.raw.test_video).apply {
      setSurface(imageReader.surface)
      isLooping = true
      start()
    }
  }

  // Cleanup MediaPlayer when composable leaves the screen
  DisposableEffect(Unit) {
    onDispose {
      mediaPlayer.stop()
      mediaPlayer.release()
      imageReader.close()
    }
  }
  val applyFilter = remember { mutableStateOf(false) }
  Column(
    modifier = modifier,
    horizontalAlignment = Alignment.CenterHorizontally,
    verticalArrangement = Arrangement.SpaceBetween,
  ) {
    GraphicsSurface(Modifier.weight(1f).fillMaxWidth(), options) { webgpu, width, height ->
      val device = webgpu.device
      var cameraTextures = arrayOf<GPUTexture>()

      // This is the buffer HardwareBufferRenderer will write to, and Dawn will read from.
      val rgbOutputAhb = HardwareBuffer.create(
        videoWidth, videoHeight,
        HardwareBuffer.RGBA_8888,
        1,
        HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE or HardwareBuffer.USAGE_GPU_COLOR_OUTPUT
      )

      val vertexData = floatArrayOf(
        -1.0f, -1.0f, 0f, 1f,
        1.0f, -1.0f, 1f, 1f,
        -1.0f, 1.0f, 0f, 0f,
        1.0f, -1.0f, 1f, 1f,
        1.0f, 1.0f, 1f, 0f,
        -1.0f, 1.0f, 0f, 0f,
      )

      val vertexBuffer = device.createBuffer(
        GPUBufferDescriptor(
          usage = BufferUsage.Vertex,
          size = vertexData.size * Float.SIZE_BYTES.toLong(),
          mappedAtCreation = true
        )
      ).apply { getMappedRange(size = size).asFloatBuffer().put(vertexData); unmap() }

      val pipeline = device.createRenderPipeline(
        GPURenderPipelineDescriptor(
          vertex = GPUVertexState(
            module = device.createShaderModule(
              GPUShaderModuleDescriptor(
                shaderSourceWGSL = GPUShaderSourceWGSL(
                  code = assets.open("cameraInput/vert.wgsl").asString()
                )
              )
            ),
            buffers = arrayOf(
              GPUVertexBufferLayout(
                arrayStride = (4 * Float.SIZE_BYTES).toLong(),
                attributes = arrayOf(
                  GPUVertexAttribute(
                    format = VertexFormat.Float32x2,
                    offset = 0,
                    shaderLocation = 0
                  ),
                  GPUVertexAttribute(
                    format = VertexFormat.Float32x2,
                    offset = (2 * Float.SIZE_BYTES).toLong(),
                    shaderLocation = 1
                  )
                ), stepMode = VertexStepMode.Undefined
              )
            )
          ),
          primitive = GPUPrimitiveState(cullMode = CullMode.None),
          fragment = GPUFragmentState(
            module = device.createShaderModule(
              GPUShaderModuleDescriptor(
                shaderSourceWGSL = GPUShaderSourceWGSL(
                  code = assets.open("cameraInput/frag.wgsl").asString()
                )
              )
            ), targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
          )
        )
      )

      val sampler = device.createSampler(
        GPUSamplerDescriptor(
          magFilter = FilterMode.Linear,
          minFilter = FilterMode.Linear
        )
      )
      val renderPassDescriptor = GPURenderPassDescriptor(
        colorAttachments = arrayOf(
          GPURenderPassColorAttachment(
            loadOp = LoadOp.Clear,
            storeOp = StoreOp.Store,
            clearValue = GPUColor(
              0.0,
              0.0,
              0.0,
              1.0
            )
          )
        )
      )

      // Load Mosaic filter
      val filterModule = device.createShaderModule(
        GPUShaderModuleDescriptor(
          shaderSourceWGSL = GPUShaderSourceWGSL(
            code = assets.open("cameraInput/mosaic.wgsl").asString()
          )
        )
      )

      var activeAhbWrapper: GPUImportedRGBHardwareBuffer? = null
      var activeYuvImage: Image? = null

      object : Renderer() {
        override suspend fun render(frameTimeNanos: Long) {
          renderPassDescriptor.colorAttachments[0].view =
            webgpu.webgpuSurface.getCurrentTexture().texture.createView()

          // Grab the latest video frame
          val newYuvImage = useImage.getAndSet(null)

          if (newYuvImage != null) {
            val yuvAhb = newYuvImage.hardwareBuffer
            if (yuvAhb != null) {

              // BLIT YUV TO RGB USING ANDROID HARDWARE
              val syncFence = convertYuvToRgbAhb(yuvAhb, rgbOutputAhb, videoWidth, videoHeight)

              // Wait for the GPU to finish the conversion before Dawn touches it
              syncFence.awaitForever()
              syncFence.close()

              // 4c. Import the now-populated RGB buffer into WebGPU (Only happens once)
              if (activeAhbWrapper == null) {
                activeAhbWrapper = DawnAhbUtil.importRgbAhb(device, rgbOutputAhb)
                val size = GPUExtent3D(width = videoWidth, height = videoHeight)

                cameraTextures = Array(2) { index ->
                  if (index == 0) activeAhbWrapper.texture else device.createTexture(
                    GPUTextureDescriptor(
                      size = size, format = TextureFormat.RGBA8Unorm,
                      usage = TextureUsage.TextureBinding or TextureUsage.StorageBinding
                    )
                  )
                }
              }
            }

            // Close the old video frame, keep the new one active
            activeYuvImage?.close()
            activeYuvImage = newYuvImage
          }

          // RUN WEBGPU RENDER PIPELINE
          if (cameraTextures.isNotEmpty() && activeAhbWrapper != null) {
            activeAhbWrapper.beginAccessNative()

            val commandBuffers = mutableListOf<GPUCommandBuffer>()
            var frameIndex = 0
            if (applyFilter.value) {
              frameIndex = 1
              // Run compute filter on the RGB Video Frame
              commandBuffers.add(cameraTextures[0].filter(device, filterModule, cameraTextures[1]))
            }

            device.createCommandEncoder().run {
              with(beginRenderPass(renderPassDescriptor)) {
                setPipeline(pipeline)
                setVertexBuffer(0, vertexBuffer)

                val bindGroup = device.createBindGroup(
                  GPUBindGroupDescriptor(
                    layout = pipeline.getBindGroupLayout(0),
                    entries = arrayOf(
                      GPUBindGroupEntry(binding = 0, sampler = sampler),
                      GPUBindGroupEntry(
                        binding = 1,
                        textureView = cameraTextures[frameIndex].createView()
                      ) // Draw filtered texture
                    )
                  )
                )

                setBindGroup(0, bindGroup)
                draw(6, 1, 0, 0)
                end()
              }
              commandBuffers.add(finish())
            }

            device.queue.submit(commandBuffers.toTypedArray())
            activeAhbWrapper.endAccessNative()
          }

          webgpu.webgpuSurface.present()
        }

        override fun close() {
          activeAhbWrapper?.close()
          activeYuvImage?.close()
          rgbOutputAhb.close()
        }
      }
    }
    Button(onClick = { applyFilter.value = !applyFilter.value }) {
      Text("Toggle Filter")
    }
  }

}

// --- HELPER FUNCTION PROVIDED BY THE PR ---
// https://github.com/androidx/media/compare/main...ychaparov:media:demo-composition_withdawn?expand=1
@RequiresApi(34)
private suspend fun convertYuvToRgbAhb(
  yuvBuffer: HardwareBuffer,
  outputRgbBuffer: HardwareBuffer,
  width: Int,
  height: Int,
): SyncFence {
  return HardwareBufferRenderer(outputRgbBuffer).use { renderer ->
    val inputBitmap = Bitmap.wrapHardwareBuffer(yuvBuffer, ColorSpace.get(ColorSpace.Named.SRGB))
      ?: throw IllegalStateException("Failed to wrap YUV AHB")

    val renderNode = RenderNode("VideoToRgbBlit")
    renderNode.setPosition(0, 0, width, height)

    val canvas = renderNode.beginRecording(width, height)
    canvas.drawBitmap(inputBitmap, 0f, 0f, null)
    renderNode.endRecording()

    renderer.setContentRoot(renderNode)

    suspendCancellableCoroutine { continuation ->
      renderer.obtainRenderRequest().draw(Executors.newSingleThreadExecutor()) { result ->
        continuation.resume(result.fence)
      }
    }
  }
}