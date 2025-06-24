#version 460

layout(binding = 0, rgba8) uniform highp writeonly image2D t_rgba8unorm;
layout(binding = 2, rgba8_snorm) uniform highp writeonly image2D t_rgba8snorm;
layout(binding = 4, rgba8ui) uniform highp writeonly uimage2D t_rgba8uint;
layout(binding = 6, rgba8i) uniform highp writeonly iimage2D t_rgba8sint;
layout(binding = 8, rgba16ui) uniform highp writeonly uimage2D t_rgba16uint;
layout(binding = 10, rgba16i) uniform highp writeonly iimage2D t_rgba16sint;
layout(binding = 12, rgba16f) uniform highp writeonly image2D t_rgba16float;
layout(binding = 14, r32ui) uniform highp writeonly uimage2D t_r32uint;
layout(binding = 16, r32i) uniform highp writeonly iimage2D t_r32sint;
layout(binding = 18, r32f) uniform highp writeonly image2D t_r32float;
layout(binding = 20, rg32ui) uniform highp writeonly uimage2D t_rg32uint;
layout(binding = 22, rg32i) uniform highp writeonly iimage2D t_rg32sint;
layout(binding = 24, rg32f) uniform highp writeonly image2D t_rg32float;
layout(binding = 26, rgba32ui) uniform highp writeonly uimage2D t_rgba32uint;
layout(binding = 28, rgba32i) uniform highp writeonly iimage2D t_rgba32sint;
layout(binding = 30, rgba32f) uniform highp writeonly image2D t_rgba32float;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint dim1 = uvec2(imageSize(t_rgba8unorm)).x;
  uint dim2 = uvec2(imageSize(t_rgba8snorm)).x;
  uint dim3 = uvec2(imageSize(t_rgba8uint)).x;
  uint dim4 = uvec2(imageSize(t_rgba8sint)).x;
  uint dim5 = uvec2(imageSize(t_rgba16uint)).x;
  uint dim6 = uvec2(imageSize(t_rgba16sint)).x;
  uint dim7 = uvec2(imageSize(t_rgba16float)).x;
  uint dim8 = uvec2(imageSize(t_r32uint)).x;
  uint dim9 = uvec2(imageSize(t_r32sint)).x;
  uint dim10 = uvec2(imageSize(t_r32float)).x;
  uint dim11 = uvec2(imageSize(t_rg32uint)).x;
  uint dim12 = uvec2(imageSize(t_rg32sint)).x;
  uint dim13 = uvec2(imageSize(t_rg32float)).x;
  uint dim14 = uvec2(imageSize(t_rgba32uint)).x;
  uint dim15 = uvec2(imageSize(t_rgba32sint)).x;
  uint dim16 = uvec2(imageSize(t_rgba32float)).x;
}
