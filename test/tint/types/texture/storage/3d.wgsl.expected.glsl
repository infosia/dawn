#version 460

layout(binding = 0, rgba8) uniform highp writeonly image3D t_rgba8unorm;
layout(binding = 2, rgba8_snorm) uniform highp writeonly image3D t_rgba8snorm;
layout(binding = 4, rgba8ui) uniform highp writeonly uimage3D t_rgba8uint;
layout(binding = 6, rgba8i) uniform highp writeonly iimage3D t_rgba8sint;
layout(binding = 8, rgba16ui) uniform highp writeonly uimage3D t_rgba16uint;
layout(binding = 10, rgba16i) uniform highp writeonly iimage3D t_rgba16sint;
layout(binding = 12, rgba16f) uniform highp writeonly image3D t_rgba16float;
layout(binding = 14, r32ui) uniform highp writeonly uimage3D t_r32uint;
layout(binding = 16, r32i) uniform highp writeonly iimage3D t_r32sint;
layout(binding = 18, r32f) uniform highp writeonly image3D t_r32float;
layout(binding = 20, rg32ui) uniform highp writeonly uimage3D t_rg32uint;
layout(binding = 22, rg32i) uniform highp writeonly iimage3D t_rg32sint;
layout(binding = 24, rg32f) uniform highp writeonly image3D t_rg32float;
layout(binding = 26, rgba32ui) uniform highp writeonly uimage3D t_rgba32uint;
layout(binding = 28, rgba32i) uniform highp writeonly iimage3D t_rgba32sint;
layout(binding = 30, rgba32f) uniform highp writeonly image3D t_rgba32float;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uvec3 dim1 = uvec3(imageSize(t_rgba8unorm));
  uvec3 dim2 = uvec3(imageSize(t_rgba8snorm));
  uvec3 dim3 = uvec3(imageSize(t_rgba8uint));
  uvec3 dim4 = uvec3(imageSize(t_rgba8sint));
  uvec3 dim5 = uvec3(imageSize(t_rgba16uint));
  uvec3 dim6 = uvec3(imageSize(t_rgba16sint));
  uvec3 dim7 = uvec3(imageSize(t_rgba16float));
  uvec3 dim8 = uvec3(imageSize(t_r32uint));
  uvec3 dim9 = uvec3(imageSize(t_r32sint));
  uvec3 dim10 = uvec3(imageSize(t_r32float));
  uvec3 dim11 = uvec3(imageSize(t_rg32uint));
  uvec3 dim12 = uvec3(imageSize(t_rg32sint));
  uvec3 dim13 = uvec3(imageSize(t_rg32float));
  uvec3 dim14 = uvec3(imageSize(t_rgba32uint));
  uvec3 dim15 = uvec3(imageSize(t_rgba32sint));
  uvec3 dim16 = uvec3(imageSize(t_rgba32float));
}
