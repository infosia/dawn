// Copyright 2021 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "src/dawn/node/binding/GPUSupportedLimits.h"

#include <utility>

namespace wgpu::binding {

////////////////////////////////////////////////////////////////////////////////
// wgpu::bindings::GPUSupportedLimits
////////////////////////////////////////////////////////////////////////////////

GPUSupportedLimits::GPUSupportedLimits(const dawn::utils::ComboLimits& limits) : limits_(limits) {}

uint32_t GPUSupportedLimits::getMaxTextureDimension1D(Napi::Env) {
    return limits_.v1.maxTextureDimension1D;
}

uint32_t GPUSupportedLimits::getMaxTextureDimension2D(Napi::Env) {
    return limits_.v1.maxTextureDimension2D;
}

uint32_t GPUSupportedLimits::getMaxTextureDimension3D(Napi::Env) {
    return limits_.v1.maxTextureDimension3D;
}

uint32_t GPUSupportedLimits::getMaxTextureArrayLayers(Napi::Env) {
    return limits_.v1.maxTextureArrayLayers;
}

uint32_t GPUSupportedLimits::getMaxBindGroups(Napi::Env) {
    return limits_.v1.maxBindGroups;
}

uint32_t GPUSupportedLimits::getMaxBindGroupsPlusVertexBuffers(Napi::Env) {
    return limits_.v1.maxBindGroupsPlusVertexBuffers;
}

uint32_t GPUSupportedLimits::getMaxBindingsPerBindGroup(Napi::Env) {
    return limits_.v1.maxBindingsPerBindGroup;
}

uint32_t GPUSupportedLimits::getMaxDynamicUniformBuffersPerPipelineLayout(Napi::Env) {
    return limits_.v1.maxDynamicUniformBuffersPerPipelineLayout;
}

uint32_t GPUSupportedLimits::getMaxDynamicStorageBuffersPerPipelineLayout(Napi::Env) {
    return limits_.v1.maxDynamicStorageBuffersPerPipelineLayout;
}

uint32_t GPUSupportedLimits::getMaxSampledTexturesPerShaderStage(Napi::Env) {
    return limits_.v1.maxSampledTexturesPerShaderStage;
}

uint32_t GPUSupportedLimits::getMaxSamplersPerShaderStage(Napi::Env) {
    return limits_.v1.maxSamplersPerShaderStage;
}

uint32_t GPUSupportedLimits::getMaxStorageBuffersPerShaderStage(Napi::Env) {
    return limits_.v1.maxStorageBuffersPerShaderStage;
}

uint32_t GPUSupportedLimits::getMaxStorageTexturesPerShaderStage(Napi::Env) {
    return limits_.v1.maxStorageTexturesPerShaderStage;
}

uint32_t GPUSupportedLimits::getMaxUniformBuffersPerShaderStage(Napi::Env) {
    return limits_.v1.maxUniformBuffersPerShaderStage;
}

uint64_t GPUSupportedLimits::getMaxUniformBufferBindingSize(Napi::Env) {
    return limits_.v1.maxUniformBufferBindingSize;
}

uint64_t GPUSupportedLimits::getMaxStorageBufferBindingSize(Napi::Env) {
    return limits_.v1.maxStorageBufferBindingSize;
}

uint32_t GPUSupportedLimits::getMinUniformBufferOffsetAlignment(Napi::Env) {
    return limits_.v1.minUniformBufferOffsetAlignment;
}

uint32_t GPUSupportedLimits::getMinStorageBufferOffsetAlignment(Napi::Env) {
    return limits_.v1.minStorageBufferOffsetAlignment;
}

uint32_t GPUSupportedLimits::getMaxVertexBuffers(Napi::Env) {
    return limits_.v1.maxVertexBuffers;
}

uint64_t GPUSupportedLimits::getMaxBufferSize(Napi::Env) {
    return limits_.v1.maxBufferSize;
}

uint32_t GPUSupportedLimits::getMaxVertexAttributes(Napi::Env) {
    return limits_.v1.maxVertexAttributes;
}

uint32_t GPUSupportedLimits::getMaxVertexBufferArrayStride(Napi::Env) {
    return limits_.v1.maxVertexBufferArrayStride;
}

uint32_t GPUSupportedLimits::getMaxInterStageShaderVariables(Napi::Env) {
    return limits_.v1.maxInterStageShaderVariables;
}

uint32_t GPUSupportedLimits::getMaxColorAttachments(Napi::Env) {
    return limits_.v1.maxColorAttachments;
}

uint32_t GPUSupportedLimits::getMaxColorAttachmentBytesPerSample(Napi::Env) {
    return limits_.v1.maxColorAttachmentBytesPerSample;
}

uint32_t GPUSupportedLimits::getMaxComputeWorkgroupStorageSize(Napi::Env) {
    return limits_.v1.maxComputeWorkgroupStorageSize;
}

uint32_t GPUSupportedLimits::getMaxComputeInvocationsPerWorkgroup(Napi::Env) {
    return limits_.v1.maxComputeInvocationsPerWorkgroup;
}

uint32_t GPUSupportedLimits::getMaxComputeWorkgroupSizeX(Napi::Env) {
    return limits_.v1.maxComputeWorkgroupSizeX;
}

uint32_t GPUSupportedLimits::getMaxComputeWorkgroupSizeY(Napi::Env) {
    return limits_.v1.maxComputeWorkgroupSizeY;
}

uint32_t GPUSupportedLimits::getMaxComputeWorkgroupSizeZ(Napi::Env) {
    return limits_.v1.maxComputeWorkgroupSizeZ;
}

uint32_t GPUSupportedLimits::getMaxComputeWorkgroupsPerDimension(Napi::Env) {
    return limits_.v1.maxComputeWorkgroupsPerDimension;
}

std::variant<uint32_t, interop::UndefinedType>
GPUSupportedLimits::getMaxStorageBuffersInFragmentStage(Napi::Env) {
    return std::variant<uint32_t, interop::UndefinedType>(
        limits_.compat.maxStorageBuffersInFragmentStage);
}

std::variant<uint32_t, interop::UndefinedType>
GPUSupportedLimits::getMaxStorageTexturesInFragmentStage(Napi::Env) {
    return std::variant<uint32_t, interop::UndefinedType>(
        limits_.compat.maxStorageTexturesInFragmentStage);
}

std::variant<uint32_t, interop::UndefinedType>
GPUSupportedLimits::getMaxStorageBuffersInVertexStage(Napi::Env) {
    return std::variant<uint32_t, interop::UndefinedType>(
        limits_.compat.maxStorageBuffersInVertexStage);
}

std::variant<uint32_t, interop::UndefinedType>
GPUSupportedLimits::getMaxStorageTexturesInVertexStage(Napi::Env) {
    return std::variant<uint32_t, interop::UndefinedType>(
        limits_.compat.maxStorageTexturesInVertexStage);
}

}  // namespace wgpu::binding
