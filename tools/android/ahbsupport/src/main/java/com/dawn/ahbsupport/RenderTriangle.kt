package com.dawn.ahbsupport

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.webgpu.BufferUsage
import androidx.webgpu.GPUBufferDescriptor
import androidx.webgpu.GPUColor
import androidx.webgpu.GPUColorTargetState
import androidx.webgpu.GPUFragmentState
import androidx.webgpu.GPUPipelineLayoutDescriptor
import androidx.webgpu.GPUPrimitiveState
import androidx.webgpu.GPURenderPassColorAttachment
import androidx.webgpu.GPURenderPassDescriptor
import androidx.webgpu.GPURenderPipelineDescriptor
import androidx.webgpu.GPUShaderModuleDescriptor
import androidx.webgpu.GPUShaderSourceWGSL
import androidx.webgpu.GPUVertexAttribute
import androidx.webgpu.GPUVertexBufferLayout
import androidx.webgpu.GPUVertexState
import androidx.webgpu.LoadOp
import androidx.webgpu.PrimitiveTopology
import androidx.webgpu.StoreOp
import androidx.webgpu.TextureFormat
import androidx.webgpu.VertexFormat
import androidx.webgpu.VertexStepMode
import androidx.webgpu.helper.asString
import com.dawn.ahbsupport.toByteBuffer
import kotlin.math.cos
import kotlin.math.sin


@Composable
fun RenderTriangle(modifier: Modifier, options: Options) {
  val context = LocalContext.current
  val assets = context.assets
  GraphicsSurface(modifier, options) { webgpu, _, _ ->
    val device = webgpu.device
    val vertexData = floatArrayOf(
      0.0f, 0.5f, 0.0f, 0.6f, 1.0f, 0.0f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.0f, 0.6f, 0.0f, 1.0f, 0.0f, 1.0f,
      0.5f, -0.5f, 0.0f, 0.6f, 0.0f, 0.0f, 1.0f, 1.0f,
    )

    val bufferSize = (vertexData.size * Float.SIZE_BYTES).toLong()

    val vertexBuffer = device.createBuffer(
      GPUBufferDescriptor(
        size = bufferSize,
        usage = BufferUsage.CopyDst or BufferUsage.Vertex,
      )
    )
    device.queue.writeBuffer(vertexBuffer, 0, vertexData.toByteBuffer())

    val vertexBufferLayout = GPUVertexBufferLayout(
      arrayStride = (8 * Float.SIZE_BYTES).toLong(),
      stepMode = VertexStepMode.Vertex,
      attributes = arrayOf(
        GPUVertexAttribute(
          format = VertexFormat.Float32x4,
          offset = 0,
          shaderLocation = 0
        ),
        GPUVertexAttribute(
          format = VertexFormat.Float32x4,
          offset = ((4 * Float.SIZE_BYTES).toLong()),
          shaderLocation = 1
        )
      )
    )

    val shaderModule = device.createShaderModule(
      GPUShaderModuleDescriptor(
        shaderSourceWGSL = GPUShaderSourceWGSL(code = assets.open("triangle.wgsl").asString())
      )
    )

    val pipelineDescriptor = GPUPipelineLayoutDescriptor()
    val pipelineLayout = device.createPipelineLayout(pipelineDescriptor)
    val renderPipeline = device.createRenderPipeline(
      GPURenderPipelineDescriptor(
        layout = pipelineLayout,
        vertex = GPUVertexState(
          module = shaderModule,
          entryPoint = "vs_main",
          buffers = arrayOf(vertexBufferLayout)
        ),
        fragment = GPUFragmentState(
          module = shaderModule,
          entryPoint = "fs_main",
          targets = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm)),
        ),
        primitive = GPUPrimitiveState(topology = PrimitiveTopology.TriangleList)
      )
    )

    object : Renderer() {
      override suspend fun render(frameTimeNanos: Long) {
        val surfaceTexture = webgpu.webgpuSurface.getCurrentTexture()
        val view = surfaceTexture.texture.createView()
        val commandEncoder = device.createCommandEncoder()
        val colorAttachment = GPURenderPassColorAttachment(
          view = view,
          loadOp = LoadOp.Clear,
          storeOp = StoreOp.Store,
          clearValue = GPUColor(0.1, 0.1, 0.1, 0.1),
        )
        val renderPassEncoder = commandEncoder.beginRenderPass(
          GPURenderPassDescriptor(
            colorAttachments = arrayOf(colorAttachment)
          )
        )
        renderPassEncoder.setPipeline(pipeline = renderPipeline)
        renderPassEncoder.setVertexBuffer(0, vertexBuffer)
        renderPassEncoder.draw(vertexCount = 3, instanceCount = 1)
        renderPassEncoder.end()

        device.queue.submit(arrayOf(commandEncoder.finish()))
        webgpu.webgpuSurface.present()
      }

      override fun close() {

      }
    }

  }
}
