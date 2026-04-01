package com.dawn.ahbsupport

import android.hardware.HardwareBuffer
import android.media.Image
import android.media.ImageReader
import android.media.MediaPlayer
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext

import androidx.webgpu.*
import androidx.webgpu.helper.DawnAhbUtil
import androidx.webgpu.helper.GPUImportedYuvAhbTexture
import androidx.webgpu.GPUSyncFence
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

  GraphicsSurface(modifier.fillMaxSize(), options) { webgpu, width, height ->
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

    val bt709Matrix = floatArrayOf(
      1.164f, 0.000f, 1.793f, 0.0f,
      1.164f, -0.213f, -0.533f, 0.0f,
      1.164f, 2.112f, 0.000f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f
    )
    val dummyView = GPUTextureView::class.java.getDeclaredConstructor(Long::class.java).apply {
      isAccessible = true
    }.newInstance(0L)

    val yuvDescriptor = GPUExternalTextureDescriptor(
      plane0 = dummyView,
      cropSize = GPUExtent2D(0, 0),
      apparentSize = GPUExtent2D(0, 0),
      srcTransferFunctionParameters = floatArrayOf(),
      dstTransferFunctionParameters = floatArrayOf(),
      gamutConversionMatrix = floatArrayOf(),
      yuvToRgbConversionMatrix = bt709Matrix,
      doYuvToRgbConversionOnly = true,
    )

    var activeYuvWrapper: GPUImportedYuvAhbTexture? = null

    object : Renderer() {
      override suspend fun render(frameTimeNanos: Long) {
        renderPassDescriptor.colorAttachments[0].view =
          webgpu.webgpuSurface.getCurrentTexture().texture.createView()

        val newYuvImage = useImage.getAndSet(null)

        if (newYuvImage != null) {
          val yuvAhb = newYuvImage.hardwareBuffer
          if (yuvAhb != null) {
            activeYuvWrapper?.close()

            activeYuvWrapper = DawnAhbUtil.importYuvAhb(device, yuvAhb, yuvDescriptor)
          }
          newYuvImage.close()
        }

        if (activeYuvWrapper != null) {
          activeYuvWrapper.beginAccess(emptyArray<GPUSyncFence>())

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
            device.queue.submit(arrayOf(finish()))
          }
          val signalFences = activeYuvWrapper.endAccess()
          // EXAMPLE: These fences signal when WebGPU is done reading from the YUV buffer.
          // In a real app, you would pass these fences back to the producer (e.g., Camera or Decoder)
          // so it knows when the buffer is free to be written to again.
          signalFences.forEach { it.close() }
        }
        webgpu.webgpuSurface.present()
      }

      override fun close() {
        activeYuvWrapper?.close()
      }
    }
  }
}