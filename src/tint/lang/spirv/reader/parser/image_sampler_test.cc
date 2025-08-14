// Copyright 2025 The Dawn & Tint Authors
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

#include "src/tint/lang/spirv/reader/parser/helper_test.h"

namespace tint::spirv::reader {
namespace {

TEST_F(SpirvParserTest, Type_Sampler) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %1 "main"
               OpExecutionMode %1 LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
       %samp = OpTypeSampler
%_ptr_Uniform = OpTypePointer UniformConstant %samp
          %5 = OpTypeFunction %void
          %6 = OpVariable %_ptr_Uniform UniformConstant
          %1 = OpFunction %void None %5
          %7 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, sampler, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_1d) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 1D 0 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 1d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_2d) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 2D 0 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_3d) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 3D 0 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 3d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_Cube) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 Cube 0 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, cube, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_SubpassData) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability InputAttachment
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 SubpassData 0 0 0 2 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, subpass_data, not_depth, non_arrayed, single_sampled, rw_op_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_Depth) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 1D 1 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 1d, depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_DepthUnknown) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 2D 2 0 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, depth_unknown, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_Arrayed) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 2D 0 1 0 1 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_RW_Ops) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 1D 0 0 0 2 Unknown
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 1d, not_depth, non_arrayed, single_sampled, rw_op_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_TexelFormat) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpCapability StorageImageExtendedFormats
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 1D 0 0 0 1 Rg32f
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 1d, not_depth, non_arrayed, single_sampled, sampling_compatible, rg32float, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_Image_TexelFormat_R8) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpCapability StorageImageExtendedFormats
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %f32 = OpTypeFloat 32
        %img = OpTypeImage %f32 1D 0 0 0 1 R8
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 1d, not_depth, non_arrayed, single_sampled, sampling_compatible, r8unorm, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_SampledImage) {
    options.sampler_mappings.insert({BindingPoint{1, 2}, BindingPoint{5, 6}});

    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpCapability StorageImageExtendedFormats
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %6 DescriptorSet 1
               OpDecorate %6 Binding 2
       %void = OpTypeVoid
        %int = OpTypeInt 32 1
        %f32 = OpTypeFloat 32
      %inner = OpTypeImage %f32 2D 0 0 0 1 Rg32f
        %img = OpTypeSampledImage %inner
    %v2float = OpTypeVector %f32 2
    %v4float = OpTypeVector %f32 4
      %int_1 = OpConstant %int 1
    %float_1 = OpConstant %f32 1
    %float_2 = OpConstant %f32 2
    %coords2 = OpConstantComposite %v2float %float_1 %float_2
%_ptr_Uniform = OpTypePointer UniformConstant %img
          %6 = OpVariable %_ptr_Uniform UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %10 = OpLoad %img %6
     %result = OpImageGather %v4float %10 %coords2 %int_1
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, sampler, read> = var undef @binding_point(5, 6)
  %2:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, rg32float, read_write>, read> = var undef @binding_point(1, 2)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %4:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, rg32float, read_write> = load %2
    %5:sampler = load %1
    %6:spirv.sampled_image<spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, rg32float, read_write>> = spirv.sampled_image<spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, rg32float, read_write>> %4, %5
    %7:vec4<f32> = spirv.image_gather %6, vec2<f32>(1.0f, 2.0f), 1i, 0u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_OpSampledImage) {
    options.sampler_mappings.insert({BindingPoint{1, 2}, BindingPoint{5, 6}});

    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability Sampled1D
               OpCapability StorageImageExtendedFormats
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 1
               OpDecorate %var_tex Binding 2
               OpDecorate %var_sampler DescriptorSet 1
               OpDecorate %var_sampler Binding 1
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
    %sampler = OpTypeSampler
%ptr_sampler = OpTypePointer UniformConstant %sampler
        %tex = OpTypeImage %float 2D 0 0 0 1 Unknown
    %ptr_tex = OpTypePointer UniformConstant %tex
%sampled_img = OpTypeSampledImage %tex
    %v2float = OpTypeVector %float 2
    %v4float = OpTypeVector %float 4
      %int_1 = OpConstant %int 1
    %float_1 = OpConstant %float 1
    %float_2 = OpConstant %float 2
    %coords2 = OpConstantComposite %v2float %float_1 %float_2
    %var_tex = OpVariable %ptr_tex UniformConstant
%var_sampler = OpVariable %ptr_sampler UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
        %sam = OpLoad %sampler %var_sampler
         %im = OpLoad %tex %var_tex
%sampled_image = OpSampledImage %sampled_img %im %sam
     %result = OpImageGather %v4float %sampled_image %coords2 %int_1
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>, read> = var undef @binding_point(1, 2)
  %2:ptr<handle, sampler, read> = var undef @binding_point(1, 1)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %4:sampler = load %2
    %5:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write> = load %1
    %6:spirv.sampled_image<spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>> = spirv.sampled_image<spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, sampling_compatible, undefined, read_write>> %5, %4
    %7:vec4<f32> = spirv.image_gather %6, vec2<f32>(1.0f, 2.0f), 1i, 0u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_DecorationNonWritable) {
    // Use 'read' access mode
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
               OpDecorate %var_tex NonWritable
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageQuerySize %v2int %im
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read> = load %1
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_DecorationNonReadable) {
    // Use 'write' access mode
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
               OpDecorate %var_tex NonReadable
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageQuerySize %v2int %im
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_NoReadsNoWrites) {
    // Use 'read_write' access mode
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageQuerySize %v2int %im
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write> = load %1
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_NoReadsNoWrites_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // Use 'write' access mode because it is more widely supported.
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageQuerySize %v2int %im
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_SomeReadsNoWrites_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // Infer 'read'
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
     %coords = OpConstantNull %v2int
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageRead %v4float %im %coords
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read> = load %1
    %4:vec4<f32> = spirv.image_read %3, vec2<i32>(0i), 0u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_NoReadsSomeWrites_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // Infer 'write'
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
     %coords = OpConstantNull %v2int
     %zero   = OpConstantNull %v4float
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
               OpImageWrite %im %coords %zero
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %4:void = spirv.image_write %3, vec2<i32>(0i), vec4<f32>(0.0f), 0u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_SomeReadsSomeWrites_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // Infer 'read_write'
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
     %coords = OpConstantNull %v2int
     %zero   = OpConstantNull %v4float
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageRead %v4float %im %coords
               OpImageWrite %im %coords %zero
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write>, read> = var undef @binding_point(0, 0)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write> = load %1
    %4:vec4<f32> = spirv.image_read %3, vec2<i32>(0i), 0u
    %5:void = spirv.image_write %3, vec2<i32>(0i), vec4<f32>(0.0f), 0u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_NoReadsNoWrites_InHelper_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // The texture operations are in a helper; the helper loads the image
    // directly.
    // Use 'write' access mode because it is more widely supported.
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
     %helper = OpFunction %void None %ep_type
%helper_entry = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageQuerySize %v2int %im
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
   %fnresult = OpFunctionCall %void %helper
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%2 = func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B3: {
    %6:void = call %2
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_SomeReadsNoWrites_InHelper_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // The texture operations are in a helper; the helper loads the image
    // directly.
    // Infer 'read'.
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
     %coords = OpConstantNull %v2int
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
     %helper = OpFunction %void None %ep_type
%helper_entry = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageRead %v4float %im %coords
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
   %fnresult = OpFunctionCall %void %helper
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read>, read> = var undef @binding_point(0, 0)
}

%2 = func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read> = load %1
    %4:vec4<f32> = spirv.image_read %3, vec2<i32>(0i), 0u
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B3: {
    %6:void = call %2
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_NoReadsSomeWrites_InHelper_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // The texture operations are in a helper; the helper loads the image
    // directly.
    // Use 'write' access mode because it is more widely supported.
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
     %coords = OpConstantNull %v2int
     %zero   = OpConstantNull %v4float
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
     %helper = OpFunction %void None %ep_type
%helper_entry = OpLabel
         %im = OpLoad %tex %var_tex
               OpImageWrite %im %coords %zero
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
   %fnresult = OpFunctionCall %void %helper
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%2 = func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %4:void = spirv.image_write %3, vec2<i32>(0i), vec4<f32>(0.0f), 0u
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B3: {
    %6:void = call %2
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Type_StorageTexture_NoDecoration_SomeReadsSomeWrites_InHelper_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // The texture operations are in a helper; the helper both reads and writes
    // the image directly.
    // Infer 'reqd_write'
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
     %coords = OpConstantNull %v2int
     %zero   = OpConstantNull %v4float
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
     %helper = OpFunction %void None %ep_type
%helper_entry = OpLabel
         %im = OpLoad %tex %var_tex
     %result = OpImageRead %v4float %im %coords
               OpImageWrite %im %coords %zero
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
   %fnresult = OpFunctionCall %void %helper
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write>, read> = var undef @binding_point(0, 0)
}

%2 = func():void {
  $B2: {
    %3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read_write> = load %1
    %4:vec4<f32> = spirv.image_read %3, vec2<i32>(0i), 0u
    %5:void = spirv.image_write %3, vec2<i32>(0i), vec4<f32>(0.0f), 0u
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B3: {
    %7:void = call %2
    ret
  }
}
)");
}

// Inference should work through function calls, but currently
// the IR-based reader does not support passing textures, or pointers
// to textures as function parameters.
TEST_F(SpirvParserTest,
       DISABLED_Type_StorageTexture_NoDecoration_NoReadsNoWrites_InHelperTextureParam_InferAccess) {
    options.infer_texture_access_mode_from_usage = true;

    // Use 'write' access mode because it is more widely supported.
    EXPECT_IR(R"(
               OpCapability Shader
               OpCapability ImageQuery
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpDecorate %var_tex DescriptorSet 0
               OpDecorate %var_tex Binding 0
       %void = OpTypeVoid
      %float = OpTypeFloat 32
        %int = OpTypeInt 32 1
        %tex = OpTypeImage %float 2D 0 0 0 2 R32f
    %ptr_tex = OpTypePointer UniformConstant %tex
      %v2int = OpTypeVector %int 2
    %v4float = OpTypeVector %float 4
    %var_tex = OpVariable %ptr_tex UniformConstant
    %ep_type = OpTypeFunction %void
%helper_type = OpTypeFunction %void %tex
     %helper = OpFunction %void None %helper_type
   %im_param = OpFunctionParameter %tex
%helper_entry = OpLabel
     %result = OpImageQuerySize %v2int %im_param
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
         %im = OpLoad %tex %var_tex
   %fnresult = OpFunctionCall %void %helper %im
               OpReturn
               OpFunctionEnd
)",
              // This is the intended result.
              // Note that the function parameter type has to be adjusted too.
              R"(
$B1: {  # root
  %1:ptr<handle, spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write>, read> = var undef @binding_point(0, 0)
}

%2 = func(%3:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, read>):void {
  $B2: {
    %4:vec2<i32> = spirv.image_query_size<i32> %3
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B3: {
    %6:spirv.image<f32, 2d, not_depth, non_arrayed, single_sampled, rw_op_compatible, r32float, write> = load %1
    %7:void = call %2, %6
    ret
  }
}
)");
}

TEST_F(
    SpirvParserTest,
    DISABLED_Type_StorageTexture_NoDecoration_SomeReadsNoWrites_InHelperTextureParam_InferAccess) {}
TEST_F(
    SpirvParserTest,
    DISABLED_Type_StorageTexture_NoDecoration_NoReadsSomeWrites_InHelperTextureParam_InferAccess) {}
TEST_F(SpirvParserTest,
       DISABLED_Type_StorageTexture_NoDecoration_SomeReadsSomeWritesInHelper_InferAccess) {}

}  // namespace
}  // namespace tint::spirv::reader
