#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec4 tint_symbol;
} v;
layout(binding = 0, r32f) uniform highp image3D arg_0;
vec4 textureLoad_272e7a() {
  vec4 res = imageLoad(arg_0, ivec3(uvec3(1u)));
  return res;
}
void main() {
  vec4 v_1 = textureLoad_272e7a();
  v.tint_symbol = v_1;
}
#version 310 es

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec4 tint_symbol;
} v;
layout(binding = 0, r32f) uniform highp image3D arg_0;
vec4 textureLoad_272e7a() {
  vec4 res = imageLoad(arg_0, ivec3(uvec3(1u)));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec4 v_1 = textureLoad_272e7a();
  v.tint_symbol = v_1;
}
