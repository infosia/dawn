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

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp

import androidx.webgpu.*
import androidx.webgpu.helper.DawnAhbUtil
import androidx.webgpu.helper.GPUImportedRGBHardwareBuffer
import androidx.webgpu.helper.asString
import androidx.compose.ui.graphics.nativeCanvas
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
  val statsText = remember { mutableStateOf("Calculating stats...") }
  val graphData = remember { mutableStateOf(floatArrayOf()) }

  Column(
    modifier = modifier,
    horizontalAlignment = Alignment.CenterHorizontally,
    verticalArrangement = Arrangement.SpaceBetween,
  ) {
    GraphicsSurface(
      Modifier
        .fillMaxWidth()
        .height(540.dp), options
    ) { webgpu, width, height ->
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

      var frameCount = 0
      var newImageCount = 0
      var sumRenderTime = 0.0
      var sumCpuSubmitTime = 0.0
      var sumCpuCopyTime = 0.0
      var sumGpuWaitTime = 0.0

      val historySize = 120
      val frameHistory = FloatArray(historySize)
      var frameHistoryIndex = 0

      object : Renderer() {
        override suspend fun render(frameTimeNanos: Long) {
          val renderStart = System.nanoTime()
          var currentCopyTime = 0.0

          renderPassDescriptor.colorAttachments[0].view =
            webgpu.webgpuSurface.getCurrentTexture().texture.createView()

          // Grab the latest video frame
          val newYuvImage = useImage.getAndSet(null)

          if (newYuvImage != null) {
            val yuvAhb = newYuvImage.hardwareBuffer
            if (yuvAhb != null) {
              newImageCount++


              // BLIT YUV TO RGB USING ANDROID HARDWARE
              val syncFence = convertYuvToRgbAhb(yuvAhb, rgbOutputAhb, videoWidth, videoHeight)

              // Wait for the GPU to finish the conversion before Dawn touches it
              val waitStart = System.nanoTime()

              syncFence.awaitForever()
              val waitEnd = System.nanoTime()

              sumGpuWaitTime += (waitEnd - waitStart) / 1_000_000.0

              syncFence.close()

              val cpuStart = System.nanoTime()

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
              val cpuEnd = System.nanoTime()

              currentCopyTime = (cpuEnd - cpuStart) / 1_000_000.0
              sumCpuCopyTime += currentCopyTime

            }

            // Close the old video frame, keep the new one active
            activeYuvImage?.close()
            activeYuvImage = newYuvImage
          }

          val submitStart = System.nanoTime()

          // RUN WEBGPU RENDER PIPELINE
          if (cameraTextures.isNotEmpty() && activeAhbWrapper != null) {
            activeAhbWrapper!!.beginAccessNative()


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
            activeAhbWrapper!!.endAccessNative()

          }
          val submitEnd = System.nanoTime()

          val currentSubmitTime = (submitEnd - submitStart) / 1_000_000.0
          sumCpuSubmitTime += currentSubmitTime

          frameHistory[frameHistoryIndex] = (currentCopyTime + currentSubmitTime).toFloat()
          frameHistoryIndex = (frameHistoryIndex + 1) % historySize


          webgpu.webgpuSurface.present()
          val renderEnd = System.nanoTime()

          sumRenderTime += (renderEnd - renderStart) / 1_000_000.0

          frameCount++

          if (frameCount >= 30) {
            val avgRender = sumRenderTime / frameCount
            val avgSubmit = sumCpuSubmitTime / frameCount
            val avgCpuCopy = if (newImageCount > 0) sumCpuCopyTime / newImageCount else 0.0

            statsText.value = """
    Total CPU Frame Time: %.2f ms
    1) RGB to WebGPU (Zero-Copy Import): %.2f ms
    2) WebGPU Draw Submit: %.2f ms
  """.trimIndent().format(
              avgRender, avgCpuCopy, avgSubmit
            )

            val orderedHistory = FloatArray(historySize)
            for (i in 0 until historySize) {
              orderedHistory[i] = frameHistory[(frameHistoryIndex + i) % historySize]
            }
            graphData.value = orderedHistory

            frameCount = 0
            newImageCount = 0
            sumRenderTime = 0.0
            sumCpuSubmitTime = 0.0
            sumCpuCopyTime = 0.0
            sumGpuWaitTime = 0.0
          }

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
    Text(statsText.value)
    ZeroCopyRGBRenderTimeGraph(
      data = graphData.value,
      modifier = Modifier
        .fillMaxWidth()
        .height(100.dp)
        .padding(16.dp)
    )

  }

}

@Composable
private fun ZeroCopyRGBRenderTimeGraph(data: FloatArray, modifier: Modifier = Modifier) {
  if (data.isEmpty()) return
  Canvas(modifier = modifier) {
    val maxOrZero = data.maxOrNull() ?: 0f
    val maxTime = if (maxOrZero > 10f) maxOrZero else 10f

    val widthPerPoint = size.width / (data.size - 1).coerceAtLeast(1)
    val height = size.height

    val path = Path()
    path.moveTo(0f, height - (data[0] / maxTime) * height)
    var peakIndex = 0
    var peakValue = data[0]

    for (i in 1 until data.size) {
      val v = data[i]
      if (v > peakValue) {
        peakValue = v
        peakIndex = i
      }
      path.lineTo(i * widthPerPoint, height - (v / maxTime) * height)
    }
    drawPath(
      path = path,
      color = Color.Green,
      style = Stroke(width = 4f)
    )

    // X and Y axes
    drawLine(Color.Gray, Offset(0f, height), Offset(size.width, height), strokeWidth = 2f) // X axis
    drawLine(Color.Gray, Offset(0f, 0f), Offset(0f, height), strokeWidth = 2f) // Y axis

    val peakX = peakIndex * widthPerPoint
    val peakY = height - (peakValue / maxTime) * height
    drawCircle(Color.Red, radius = 8f, center = Offset(peakX, peakY))

    drawContext.canvas.nativeCanvas.apply {
      // Y axis max text
      drawText(
        String.format("%.1f ms", maxTime),
        10f,
        35f,
        android.graphics.Paint().apply {
          color = android.graphics.Color.WHITE
          textSize = 30f
        }
      )
      // X axis max text
      drawText(
        "${data.size}",
        size.width - 60f,
        height - 10f,
        android.graphics.Paint().apply {
          color = android.graphics.Color.WHITE
          textSize = 30f
        }
      )

      // Peak value
      var textX = peakX
      if (textX > size.width - 250f) textX = size.width - 250f
      var textY = peakY - 15f
      if (textY < 30f) textY = peakY + 45f

      drawText(
        String.format("Peak: %.1f ms at X:%d", peakValue, peakIndex),
        textX,
        textY,
        android.graphics.Paint().apply {
          color = android.graphics.Color.RED
          textSize = 30f
        }
      )
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