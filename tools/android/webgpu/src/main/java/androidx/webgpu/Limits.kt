package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class Limits(
    public var maxTextureDimension1D: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxTextureDimension2D: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxTextureDimension3D: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxTextureArrayLayers: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxBindGroups: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxBindGroupsPlusVertexBuffers: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxBindingsPerBindGroup: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxDynamicUniformBuffersPerPipelineLayout: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxDynamicStorageBuffersPerPipelineLayout: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxSampledTexturesPerShaderStage: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxSamplersPerShaderStage: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxStorageBuffersPerShaderStage: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxStorageTexturesPerShaderStage: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxUniformBuffersPerShaderStage: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxUniformBufferBindingSize: Long = Constants.LIMIT_U64_UNDEFINED,
    public var maxStorageBufferBindingSize: Long = Constants.LIMIT_U64_UNDEFINED,
    public var minUniformBufferOffsetAlignment: Int = Constants.LIMIT_U32_UNDEFINED,
    public var minStorageBufferOffsetAlignment: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxVertexBuffers: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxBufferSize: Long = Constants.LIMIT_U64_UNDEFINED,
    public var maxVertexAttributes: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxVertexBufferArrayStride: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxInterStageShaderVariables: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxColorAttachments: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxColorAttachmentBytesPerSample: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeWorkgroupStorageSize: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeInvocationsPerWorkgroup: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeWorkgroupSizeX: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeWorkgroupSizeY: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeWorkgroupSizeZ: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxComputeWorkgroupsPerDimension: Int = Constants.LIMIT_U32_UNDEFINED,
    public var maxImmediateSize: Int = Constants.LIMIT_U32_UNDEFINED,
)
