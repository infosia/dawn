package com.webgpu.demo

import androidx.webgpu.GPUBindGroupDescriptor
import androidx.webgpu.GPUBindGroupEntry
import androidx.webgpu.GPUBindGroupLayoutDescriptor
import androidx.webgpu.GPUBindGroupLayoutEntry
import androidx.webgpu.GPUComputePipelineDescriptor
import androidx.webgpu.GPUComputeState
import androidx.webgpu.GPUDevice
import androidx.webgpu.GPUShaderModule
import androidx.webgpu.GPUTexture
import androidx.webgpu.GPUPipelineLayoutDescriptor
import androidx.webgpu.ShaderStage
import androidx.webgpu.StorageTextureAccess
import androidx.webgpu.GPUStorageTextureBindingLayout
import androidx.webgpu.GPUTextureBindingLayout
import androidx.webgpu.TextureSampleType
import androidx.webgpu.GPUCommandBuffer

/**
 * Extension function to apply a filter to a texture using a compute shader.
 *
 * @param device The Dawn device to use for creating resources.
 * @param shaderModule The compiled shader module containing the filter logic.
 * @param outputTexture The texture to write the filtered output to.
 * @return A GPUCommandBuffer containing the compute pass commands.
 */
fun GPUTexture.filter(device: GPUDevice, shaderModule: GPUShaderModule, outputTexture: GPUTexture): GPUCommandBuffer {
    val bindGroupLayout = device.createBindGroupLayout(
        GPUBindGroupLayoutDescriptor(
            entries = arrayOf(
                // Input texture (read-only)
                GPUBindGroupLayoutEntry(
                    binding = 0,
                    visibility = ShaderStage.Compute,
                    texture = GPUTextureBindingLayout(sampleType = TextureSampleType.Float)
                ),
                // Output texture (write-only)
                GPUBindGroupLayoutEntry(
                    binding = 1,
                    visibility = ShaderStage.Compute,
                    storageTexture = GPUStorageTextureBindingLayout(
                        access = StorageTextureAccess.WriteOnly,
                        format = format
                    )
                )
            )
        )
    )

    // Create a command encoder to record GPU commands.
    val commandEncoder = device.createCommandEncoder()
    commandEncoder.beginComputePass().use { computePassEncoder ->
        // Create a compute pipeline with the shader and layout.
        val pipeline = device.createComputePipeline(
            GPUComputePipelineDescriptor(
                // Create a pipeline layout from the bind group layout.
                layout = device.createPipelineLayout(
                    GPUPipelineLayoutDescriptor(
                        bindGroupLayouts = arrayOf(bindGroupLayout)
                    )
                ), compute = GPUComputeState(module = shaderModule)
            )
        )
        // Set the active compute pipeline.
        computePassEncoder.setPipeline(pipeline)
        // Create a bind group that binds the actual textures to the layout.
        // Bind group 0 is the first (and only) bind group layout in the pipeline layout.
        computePassEncoder.setBindGroup(
            0,
            device.createBindGroup(
                GPUBindGroupDescriptor(
                    layout = pipeline.getBindGroupLayout(0),
                    entries = arrayOf(
                        // Bind the input texture view to binding 0.
                        GPUBindGroupEntry(
                            binding = 0,
                            textureView = createView()
                        ),
                        // Bind the output texture view to binding 1.
                        GPUBindGroupEntry(
                            binding = 1,
                            textureView = outputTexture.createView()
                        )
                    )
                )
            )
        )
        // Define the size of each workgroup in the compute shader.
        // These values should match the workgroup size defined in the shader.
        val workgroupSizeX = 8
        val workgroupSizeY = 8
        computePassEncoder.dispatchWorkgroups(
            (width + workgroupSizeX - 1) / workgroupSizeX,
            (height + workgroupSizeY - 1) / workgroupSizeY
        )
        computePassEncoder.end()
    }
    bindGroupLayout.close()
    return commandEncoder.finish()
}
