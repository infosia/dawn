package com.dawn.ahbsupport

import android.graphics.BitmapFactory
import android.graphics.PixelFormat
import android.hardware.HardwareBuffer
import android.media.Image
import android.media.ImageReader
import android.os.Build
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext

// WebGPU imports
import androidx.webgpu.*
import androidx.webgpu.helper.GPUAndroidHardwareBufferUtil
import androidx.webgpu.helper.GPUHardwareBufferTexture
import androidx.webgpu.helper.GPUSyncFence
import androidx.webgpu.helper.asString
import java.util.concurrent.atomic.AtomicReference

@Composable
fun ZeroCopyRGBAImage(modifier: Modifier, options: Options) {
  val context = LocalContext.current
  val assets = context.assets

  GraphicsSurface(modifier.fillMaxSize(), options) { webgpu, width, height ->
    val device = webgpu.device
    var filterTextures = arrayOf<GPUTexture>()

    // LOAD IMAGE INTO A ZERO-COPY HARDWARE BUFFER
    val bitmap = BitmapFactory.decodeResource(context.resources, R.drawable.test_image)

    val imageReader = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      ImageReader.newInstance(
        bitmap.width, bitmap.height, PixelFormat.RGBA_8888, 1, HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE
      )
    } else {
      ImageReader.newInstance(bitmap.width, bitmap.height, PixelFormat.RGBA_8888, 1)
    }

    val useImage = AtomicReference<Image?>(null)

    // Wait for the buffer to be ready asynchronously
    imageReader.setOnImageAvailableListener({ reader ->
                                              val image = reader.acquireLatestImage()
                                              if (image != null) {
                                                useImage.getAndSet(image)?.close()
                                              }
                                            }, null)

    // Draw the image to the Android Hardware Canvas
    val canvas = imageReader.surface.lockHardwareCanvas()
    canvas.drawColor(android.graphics.Color.WHITE) // Prevent transparent black backgrounds
    canvas.drawBitmap(bitmap, 0f, 0f, null)
    imageReader.surface.unlockCanvasAndPost(canvas)

    val vertexData = floatArrayOf(
      // X,     Y,     U,  V
      -1.0f, -1.0f,   0f, 1f,
      1.0f, -1.0f,   1f, 1f,
      -1.0f,  1.0f,   0f, 0f,
      1.0f, -1.0f,   1f, 1f,
      1.0f,  1.0f,   1f, 0f,
      -1.0f,  1.0f,   0f, 0f,
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
            GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = assets.open("cameraInput/vert.wgsl").asString()))
          ),
          buffers = arrayOf(
            GPUVertexBufferLayout(
              arrayStride = (4 * Float.SIZE_BYTES).toLong(), // 4 floats per vertex (X,Y,U,V)
              attributes = arrayOf(
                GPUVertexAttribute(format = VertexFormat.Float32x2, offset = 0, shaderLocation = 0), // XY
                GPUVertexAttribute(format = VertexFormat.Float32x2, offset = (2 * Float.SIZE_BYTES).toLong(), shaderLocation = 1)  // UV
              ),
              stepMode = VertexStepMode.Undefined
            )
          )
        ),
        primitive = GPUPrimitiveState(cullMode = CullMode.None),
        fragment = GPUFragmentState(
          module = device.createShaderModule(
            GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = assets.open("cameraInput/frag.wgsl").asString()))
          ),
          targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm))
        )
      )
    )

    val sampler = device.createSampler(GPUSamplerDescriptor(magFilter = FilterMode.Linear, minFilter = FilterMode.Linear))

    val renderPassDescriptor = GPURenderPassDescriptor(
      colorAttachments = arrayOf(
        GPURenderPassColorAttachment(loadOp = LoadOp.Clear, storeOp = StoreOp.Store, clearValue = GPUColor(0.0, 0.0, 0.0, 1.0))
      )
    )

    // Load the Compute Filter Shader
    val filterModule = device.createShaderModule(
      GPUShaderModuleDescriptor(shaderSourceWGSL = GPUShaderSourceWGSL(code = assets.open("cameraInput/mosaic.wgsl").asString()))
    )

    var activeAhbWrapper: GPUHardwareBufferTexture? = null
    var activeImage: Image? = null

    object : Renderer() {
      override suspend fun render(frameTimeNanos: Long) {
        // Target the screen
        renderPassDescriptor.colorAttachments[0].view =
          webgpu.webgpuSurface.getCurrentTexture().texture.createView()

        // Check if the asynchronous image buffer has arrived
        val newImage = useImage.getAndSet(null)
        if (newImage != null && activeAhbWrapper == null) {
          val ahb = newImage.hardwareBuffer
          if (ahb != null) {
            activeImage = newImage
            activeAhbWrapper = GPUAndroidHardwareBufferUtil.createTexture(device, ahb)

            val size = GPUExtent3D(width = ahb.width, height = ahb.height)

            // Index 0: Raw AHB Texture | Index 1: Filtered Output Texture
            filterTextures = Array(2) { index ->
              if (index == 0) activeAhbWrapper!!.texture else device.createTexture(
                GPUTextureDescriptor(
                  size = size, format = TextureFormat.RGBA8Unorm,
                  usage = TextureUsage.TextureBinding or TextureUsage.StorageBinding
                )
              )
            }
          }
        }

        if (filterTextures.isNotEmpty() && activeAhbWrapper != null) {

          // Tell Vulkan we are about to read the Android memory
          activeAhbWrapper!!.beginAccess(emptyArray<GPUSyncFence>())

          val commandBuffers = mutableListOf<GPUCommandBuffer>()

          // Run the compute shader (Reads from 0, writes to 1)
          commandBuffers.add(filterTextures[0].filter(device, filterModule, filterTextures[1]))

          device.createCommandEncoder().run {
            with(beginRenderPass(renderPassDescriptor)) {
              setPipeline(pipeline)
              setVertexBuffer(0, vertexBuffer)

              val bindGroup = device.createBindGroup(
                GPUBindGroupDescriptor(
                  layout = pipeline.getBindGroupLayout(0),
                  entries = arrayOf(
                    GPUBindGroupEntry(binding = 0, sampler = sampler),
                    GPUBindGroupEntry(binding = 1, textureView = filterTextures[1].createView()) // Bind the filtered output!
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

          val signalFences = activeAhbWrapper!!.endAccess()
          // EXAMPLE: These fences signal when WebGPU is done reading from the buffer.
          // In a real app, you would pass these fences back to the producer (e.g., Camera or Decoder)
          // so it knows when the buffer is free to be written to again.
          signalFences.forEach { it.close() }
        }

        webgpu.webgpuSurface.present()
      }

      override fun close() {
        activeAhbWrapper?.close()
        activeImage?.close()
        imageReader.close()
        bitmap.recycle()
      }
    }
  }
}