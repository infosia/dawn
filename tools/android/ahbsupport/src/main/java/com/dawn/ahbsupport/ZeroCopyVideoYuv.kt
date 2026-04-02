package com.dawn.ahbsupport

import android.hardware.HardwareBuffer
import android.media.Image
import android.media.ImageReader
import android.media.MediaPlayer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

import androidx.webgpu.*
import androidx.webgpu.helper.GPUAndroidHardwareBufferUtil
import androidx.webgpu.helper.GPUHardwareBufferExternalTexture
import androidx.webgpu.helper.GPUSyncFence
import androidx.webgpu.helper.asString
import java.util.concurrent.atomic.AtomicReference

@RequiresApi(Build.VERSION_CODES.Q)
@Composable
fun ZeroCopyVideoYuv(modifier: Modifier, options: Options) {
  val context = LocalContext.current
  val assets = context.assets

  val videoWidth = 1280
  val videoHeight = 720

  val useImage = remember { AtomicReference<Image?>(null) }

  // State for the UI
  val applyFilter = remember { mutableStateOf(false) }
  val statsText = remember { mutableStateOf("Calculating stats...") }
  val graphData = remember { mutableStateOf(floatArrayOf()) }

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
                                        useImage.getAndSet(image)?.close()
                                      }
                                    } catch (e: IllegalStateException) {
                                      // Ignore: Image queue is full or closed
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

  DisposableEffect(Unit) {
    onDispose {
      mediaPlayer.stop()
      mediaPlayer.release()
      imageReader.close()
    }
  }

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

      val vertexData = floatArrayOf(
        -1.0f, -1.0f, 0f, 1f, 1.0f, -1.0f, 1f, 1f, -1.0f, 1.0f, 0f, 0f,
        1.0f, -1.0f, 1f, 1f, 1.0f, 1.0f, 1f, 0f, -1.0f, 1.0f, 0f, 0f,
      )

      val vertexBuffer = device.createBuffer(
        GPUBufferDescriptor(
          usage = BufferUsage.Vertex,
          size = vertexData.size * Float.SIZE_BYTES.toLong(),
          mappedAtCreation = true
        )
      ).apply { getMappedRange(size = size).asFloatBuffer().put(vertexData); unmap() }

      // 1. STANDARD YUV PIPELINE
      val pipelineNormal = device.createRenderPipeline(
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
                  GPUVertexAttribute(format = VertexFormat.Float32x2, offset = 0, shaderLocation = 0),
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
                  code = assets.open("cameraInput/frag_yuv.wgsl").asString()
                )
              )
            ),
            targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
          )
        )
      )

      // 2. MOSAIC FILTER YUV PIPELINE
      val pipelineFiltered = device.createRenderPipeline(
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
                  GPUVertexAttribute(format = VertexFormat.Float32x2, offset = 0, shaderLocation = 0),
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
                  code = assets.open("cameraInput/frag_yuv_mosaic.wgsl").asString()
                )
              )
            ),
            targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
          )
        )
      )

      val sampler = device.createSampler(
        GPUSamplerDescriptor(
          magFilter = FilterMode.Linear,
          minFilter = FilterMode.Linear
        )
      )

      val dummyView = GPUTextureView::class.java.getDeclaredConstructor(Long::class.java).apply {
        isAccessible = true
      }.newInstance(0L)

      val yuvDescriptor = GPUExternalTextureDescriptor(
        plane0 = dummyView,
        cropSize = GPUExtent2D(600, 600),
        srcColorSpace = GPUColorSpaceDawn(
          primaries = ColorSpacePrimariesDawn.Rec709,
          transfer = ColorSpaceTransferDawn.SRGB,
          yCbCrRange = ColorSpaceYCbCrRangeDawn.Narrow,
          yCbCrMatrix = ColorSpaceYCbCrMatrixDawn.Rec709
        ),
        dstColorSpace = PredefinedColorSpace.SRGB,
        rotation = ExternalTextureRotation.Rotate90Degrees,
      )

      var activeYuvWrapper: GPUHardwareBufferExternalTexture? = null
      var activeYuvImage: Image? = null

      var frameCount = 0
      var sumRenderTime = 0.0
      var sumCpuOverhead = 0.0
      var lastFpsTime = System.nanoTime()

      val historySize = 120
      val frameHistory = FloatArray(historySize)
      var frameHistoryIndex = 0

      object : Renderer() {
        override suspend fun render(frameTimeNanos: Long) {
          val renderStart = System.nanoTime()
          var currentSetupTime = 0.0

          // --- FIX: Create the renderPassDescriptor HERE inside the loop ---
          val currentSurfaceView = webgpu.webgpuSurface.getCurrentTexture().texture.createView()

          val renderPassDescriptor = GPURenderPassDescriptor(
            colorAttachments = arrayOf(
              GPURenderPassColorAttachment(
                view = currentSurfaceView, // Assign the view directly in the constructor!
                loadOp = LoadOp.Clear,
                storeOp = StoreOp.Store,
                clearValue = GPUColor(0.0, 0.0, 0.0, 1.0)
              )
            )
          )

          val newYuvImage = useImage.getAndSet(null)

          if (newYuvImage != null) {
            activeYuvImage?.close()
            activeYuvImage = newYuvImage

            val yuvAhb = newYuvImage.hardwareBuffer
            if (yuvAhb != null) {
              activeYuvWrapper?.close()

              val setupStart = System.nanoTime()
              activeYuvWrapper = GPUAndroidHardwareBufferUtil.createExternalTexture(device, yuvAhb, yuvDescriptor)
              val setupEnd = System.nanoTime()
              currentSetupTime = (setupEnd - setupStart) / 1_000_000.0
            }
          }

          if (activeYuvWrapper != null) {
            activeYuvWrapper!!.beginAccess(emptyArray<GPUSyncFence>())

            val activePipeline = if (applyFilter.value) pipelineFiltered else pipelineNormal

            device.createCommandEncoder().run {
              with(beginRenderPass(renderPassDescriptor)) {
                setPipeline(activePipeline)
                setVertexBuffer(0, vertexBuffer)

                val bindGroup = device.createBindGroup(
                  GPUBindGroupDescriptor(
                    layout = activePipeline.getBindGroupLayout(0),
                    entries = arrayOf(
                      GPUBindGroupEntry(binding = 0, sampler = sampler),
                      GPUBindGroupEntry(
                        binding = 1,
                        externalTextureBindingEntry = GPUExternalTextureBindingEntry(
                          activeYuvWrapper.externalTexture
                        )
                      )
                    )
                  )
                )

                setBindGroup(0, bindGroup)
                draw(6, 1, 0, 0)
                end()
              }
              val submitStart = System.nanoTime()
              device.queue.submit(arrayOf(finish()))
              val submitEnd = System.nanoTime()
              val currentSubmitTime = (submitEnd - submitStart) / 1_000_000.0

              val totalCpuOverhead = currentSetupTime + currentSubmitTime
              sumCpuOverhead += totalCpuOverhead

              frameHistory[frameHistoryIndex] = totalCpuOverhead.toFloat()
              frameHistoryIndex = (frameHistoryIndex + 1) % historySize
            }
            val signalFences = activeYuvWrapper.endAccess()
            signalFences.forEach { it.close() }
          }

          webgpu.webgpuSurface.present()
          val renderEnd = System.nanoTime()
          sumRenderTime += (renderEnd - renderStart) / 1_000_000.0
          frameCount++

          if (frameCount >= 30) {
            val now = System.nanoTime()
            val fps = 30 * 1_000_000_000.0 / (now - lastFpsTime)
            lastFpsTime = now

            val avgRender = sumRenderTime / frameCount
            val avgCpu = sumCpuOverhead / frameCount

            statsText.value = """
                FPS: %.1f
                Render Loop Time: %.2f ms
                CPU Overhead (YUV): %.2f ms
            """.trimIndent().format(fps, avgRender, avgCpu)

            val orderedHistory = FloatArray(historySize)
            for (i in 0 until historySize) {
              orderedHistory[i] = frameHistory[(frameHistoryIndex + i) % historySize]
            }
            graphData.value = orderedHistory

            frameCount = 0
            sumRenderTime = 0.0
            sumCpuOverhead = 0.0
          }
        }

        override fun close() {
          activeYuvWrapper?.close()
          activeYuvImage?.close()
        }
      }
    }

    Button(onClick = { applyFilter.value = !applyFilter.value }) {
      Text(if (applyFilter.value) "Disable Mosaic Filter" else "Enable Mosaic Filter")
    }

    Text(statsText.value)

    ZeroCopyYUVRenderTimeGraph(
      data = graphData.value,
      modifier = Modifier
        .fillMaxWidth()
        .height(180.dp)
        .padding(16.dp)
    )
  }
}

@Composable
private fun ZeroCopyYUVRenderTimeGraph(data: FloatArray, modifier: Modifier = Modifier) {
  if (data.isEmpty()) return
  val paintBlack30 = remember {
    android.graphics.Paint().apply {
      color = android.graphics.Color.BLACK
      textSize = 30f
    }
  }
  val paintBlack25 = remember {
    android.graphics.Paint().apply {
      color = android.graphics.Color.BLACK
      textSize = 25f
    }
  }
  val paintLabelCenter = remember {
    android.graphics.Paint().apply {
      color = android.graphics.Color.BLACK
      textSize = 25f
      textAlign = android.graphics.Paint.Align.CENTER
    }
  }
  val paintRed30 = remember {
    android.graphics.Paint().apply {
      color = android.graphics.Color.RED
      textSize = 30f
    }
  }

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
        paintBlack30
      )
      // Y axis label
      drawText(
        "Time (ms)",
        10f,
        75f,
        paintBlack25
      )

      // X axis max text
      drawText(
        "${data.size}",
        size.width - 60f,
        height - 10f,
        paintBlack30
      )
      // X axis label
      drawText(
        "Frames",
        size.width / 2f,
        height - 10f,
        paintLabelCenter
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
        paintRed30
      )
    }
  }
}
