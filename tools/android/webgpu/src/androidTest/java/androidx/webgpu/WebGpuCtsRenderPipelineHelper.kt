package androidx.webgpu

import org.junit.Assert.fail

object WebGpuCtsRenderPipelineHelper {

    val kDefaultVertexShaderCode = """
        @vertex fn main() -> @builtin(position) vec4<f32> {
            return vec4<f32>(0.0, 0.0, 0.0, 1.0);
        }
    """.trimIndent()

    val kDefaultFragmentShaderCode = """
        @fragment fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(0.0, 1.0, 0.0, 1.0);
        }
    """.trimIndent()

    fun getDescriptor(
        device: GPUDevice,
        primitive: GPUPrimitiveState = GPUPrimitiveState(topology = PrimitiveTopology.TriangleList),
        targets: Array<GPUColorTargetState> = arrayOf(GPUColorTargetState(format = TextureFormat.RGBA8Unorm)),
        multisample: GPUMultisampleState = GPUMultisampleState(),
        depthStencil: GPUDepthStencilState? = null,
        fragmentShaderCode: String = kDefaultFragmentShaderCode,
        noFragment: Boolean = false,
        vertexState: GPUVertexState? = null,
        fragmentConstants: Array<GPUConstantEntry> = arrayOf()
    ): GPURenderPipelineDescriptor {

        val resolvedVertexState = vertexState ?: GPUVertexState(
            module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = kDefaultVertexShaderCode)
                )
            ),
            entryPoint = "main",
            buffers = arrayOf()
        )

        val resolvedFragmentState = if (noFragment) null else GPUFragmentState(
            module = device.createShaderModule(
                GPUShaderModuleDescriptor(
                    shaderSourceWGSL = GPUShaderSourceWGSL(code = fragmentShaderCode)
                )
            ),
            entryPoint = "main",
            targets = targets,
            constants = fragmentConstants
        )

        return GPURenderPipelineDescriptor(
            vertex = resolvedVertexState,
            fragment = resolvedFragmentState,
            primitive = primitive,
            depthStencil = depthStencil,
            multisample = multisample,
            layout = device.createPipelineLayout(GPUPipelineLayoutDescriptor(bindGroupLayouts = arrayOf()))
        )
    }

    suspend fun doCreateRenderPipelineTest(
        device: GPUDevice,
        isAsync: Boolean,
        success: Boolean,
        descriptorBuilder: () -> GPURenderPipelineDescriptor
    ) {
        device.pushErrorScope(ErrorFilter.Validation)

        try {
            val descriptor = descriptorBuilder()
            if (isAsync) {
                device.createRenderPipelineAndAwait(descriptor)
            } else {
                device.createRenderPipeline(descriptor)
            }

            val error = device.popErrorScope()
            // If we expect success, we should get here and error should be ErrorType.NoError
            if (!success) {
                // If we expected failure, we SHOULD have thrown an exception! Let's mock failure if we reach here
                fail("Expected ValidationException but got success")
            }
        } catch (e: Exception) {
            // Validation error was thrown (from either popErrorScope or createRenderPipelineAndAwait)
            if (success) {
                fail("Expected success but got exception: ${e.message}")
            }
            // If !success, we caught the expected Exception!
        }
    }
}
