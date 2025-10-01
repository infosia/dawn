//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;


struct tint_GammaTransferParams {
  float G;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  uint padding;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_GammaTransferParams gammaDecodeParams;
  tint_GammaTransferParams gammaEncodeParams;
  mat3 gamutConversionMatrix;
  mat3x2 sampleTransform;
  mat3x2 loadTransform;
  vec2 samplePlane0RectMin;
  vec2 samplePlane0RectMax;
  vec2 samplePlane1RectMin;
  vec2 samplePlane1RectMax;
  uvec2 apparentSize;
  vec2 plane1CoordFactor;
};

layout(binding = 0, std430)
buffer f_prevent_dce_block_ssbo {
  uvec2 inner;
} v;
uniform highp sampler2D f_arg_0_plane0;
uniform highp sampler2D f_arg_0_plane1;
layout(binding = 3, std140)
uniform f_arg_0_params_block_ubo {
  uvec4 inner[17];
} v_1;
mat3x2 v_2(uint start_byte_offset) {
  uvec4 v_3 = v_1.inner[(start_byte_offset / 16u)];
  vec2 v_4 = uintBitsToFloat(mix(v_3.xy, v_3.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_5 = v_1.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_6 = uintBitsToFloat(mix(v_5.xy, v_5.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_7 = v_1.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_4, v_6, uintBitsToFloat(mix(v_7.xy, v_7.zw, bvec2(((((16u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
mat3 v_8(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_GammaTransferParams v_9(uint start_byte_offset) {
  return tint_GammaTransferParams(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) % 16u) / 4u)]), v_1.inner[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) % 16u) / 4u)]);
}
mat3x4 v_10(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_11(uint start_byte_offset) {
  uint v_12 = v_1.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)];
  uint v_13 = v_1.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)];
  mat3x4 v_14 = v_10((16u + start_byte_offset));
  tint_GammaTransferParams v_15 = v_9((64u + start_byte_offset));
  tint_GammaTransferParams v_16 = v_9((96u + start_byte_offset));
  mat3 v_17 = v_8((128u + start_byte_offset));
  mat3x2 v_18 = v_2((176u + start_byte_offset));
  mat3x2 v_19 = v_2((200u + start_byte_offset));
  uvec4 v_20 = v_1.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_21 = uintBitsToFloat(mix(v_20.xy, v_20.zw, bvec2(((((224u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_22 = v_1.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_23 = uintBitsToFloat(mix(v_22.xy, v_22.zw, bvec2(((((232u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_24 = v_1.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_25 = uintBitsToFloat(mix(v_24.xy, v_24.zw, bvec2(((((240u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_26 = v_1.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_27 = uintBitsToFloat(mix(v_26.xy, v_26.zw, bvec2(((((248u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_28 = v_1.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_29 = mix(v_28.xy, v_28.zw, bvec2(((((256u + start_byte_offset) % 16u) / 4u) == 2u)));
  uvec4 v_30 = v_1.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_12, v_13, v_14, v_15, v_16, v_17, v_18, v_19, v_21, v_23, v_25, v_27, v_29, uintBitsToFloat(mix(v_30.xy, v_30.zw, bvec2(((((264u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
uvec2 textureDimensions_cdc6c9() {
  uvec2 res = (v_11(0u).apparentSize + uvec2(1u));
  return res;
}
void main() {
  v.inner = textureDimensions_cdc6c9();
}
//
// compute_main
//
#version 310 es


struct tint_GammaTransferParams {
  float G;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  uint padding;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_GammaTransferParams gammaDecodeParams;
  tint_GammaTransferParams gammaEncodeParams;
  mat3 gamutConversionMatrix;
  mat3x2 sampleTransform;
  mat3x2 loadTransform;
  vec2 samplePlane0RectMin;
  vec2 samplePlane0RectMax;
  vec2 samplePlane1RectMin;
  vec2 samplePlane1RectMax;
  uvec2 apparentSize;
  vec2 plane1CoordFactor;
};

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  uvec2 inner;
} v;
uniform highp sampler2D arg_0_plane0;
uniform highp sampler2D arg_0_plane1;
layout(binding = 3, std140)
uniform arg_0_params_block_1_ubo {
  uvec4 inner[17];
} v_1;
mat3x2 v_2(uint start_byte_offset) {
  uvec4 v_3 = v_1.inner[(start_byte_offset / 16u)];
  vec2 v_4 = uintBitsToFloat(mix(v_3.xy, v_3.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_5 = v_1.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_6 = uintBitsToFloat(mix(v_5.xy, v_5.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_7 = v_1.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_4, v_6, uintBitsToFloat(mix(v_7.xy, v_7.zw, bvec2(((((16u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
mat3 v_8(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_GammaTransferParams v_9(uint start_byte_offset) {
  return tint_GammaTransferParams(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v_1.inner[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) % 16u) / 4u)]), v_1.inner[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) % 16u) / 4u)]);
}
mat3x4 v_10(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_11(uint start_byte_offset) {
  uint v_12 = v_1.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)];
  uint v_13 = v_1.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)];
  mat3x4 v_14 = v_10((16u + start_byte_offset));
  tint_GammaTransferParams v_15 = v_9((64u + start_byte_offset));
  tint_GammaTransferParams v_16 = v_9((96u + start_byte_offset));
  mat3 v_17 = v_8((128u + start_byte_offset));
  mat3x2 v_18 = v_2((176u + start_byte_offset));
  mat3x2 v_19 = v_2((200u + start_byte_offset));
  uvec4 v_20 = v_1.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_21 = uintBitsToFloat(mix(v_20.xy, v_20.zw, bvec2(((((224u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_22 = v_1.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_23 = uintBitsToFloat(mix(v_22.xy, v_22.zw, bvec2(((((232u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_24 = v_1.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_25 = uintBitsToFloat(mix(v_24.xy, v_24.zw, bvec2(((((240u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_26 = v_1.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_27 = uintBitsToFloat(mix(v_26.xy, v_26.zw, bvec2(((((248u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_28 = v_1.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_29 = mix(v_28.xy, v_28.zw, bvec2(((((256u + start_byte_offset) % 16u) / 4u) == 2u)));
  uvec4 v_30 = v_1.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_12, v_13, v_14, v_15, v_16, v_17, v_18, v_19, v_21, v_23, v_25, v_27, v_29, uintBitsToFloat(mix(v_30.xy, v_30.zw, bvec2(((((264u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
uvec2 textureDimensions_cdc6c9() {
  uvec2 res = (v_11(0u).apparentSize + uvec2(1u));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = textureDimensions_cdc6c9();
}
//
// vertex_main
//
#version 310 es


struct tint_GammaTransferParams {
  float G;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  uint padding;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_GammaTransferParams gammaDecodeParams;
  tint_GammaTransferParams gammaEncodeParams;
  mat3 gamutConversionMatrix;
  mat3x2 sampleTransform;
  mat3x2 loadTransform;
  vec2 samplePlane0RectMin;
  vec2 samplePlane0RectMax;
  vec2 samplePlane1RectMin;
  vec2 samplePlane1RectMax;
  uvec2 apparentSize;
  vec2 plane1CoordFactor;
};

struct VertexOutput {
  vec4 pos;
  uvec2 prevent_dce;
};

uniform highp sampler2D v_arg_0_plane0;
uniform highp sampler2D v_arg_0_plane1;
layout(binding = 2, std140)
uniform v_arg_0_params_block_ubo {
  uvec4 inner[17];
} v;
layout(location = 0) flat out uvec2 tint_interstage_location0;
mat3x2 v_1(uint start_byte_offset) {
  uvec4 v_2 = v.inner[(start_byte_offset / 16u)];
  vec2 v_3 = uintBitsToFloat(mix(v_2.xy, v_2.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_4 = v.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_5 = uintBitsToFloat(mix(v_4.xy, v_4.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_6 = v.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_3, v_5, uintBitsToFloat(mix(v_6.xy, v_6.zw, bvec2(((((16u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
mat3 v_7(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_GammaTransferParams v_8(uint start_byte_offset) {
  return tint_GammaTransferParams(uintBitsToFloat(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]), uintBitsToFloat(v.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v.inner[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v.inner[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v.inner[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) % 16u) / 4u)]), uintBitsToFloat(v.inner[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) % 16u) / 4u)]), v.inner[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) % 16u) / 4u)]);
}
mat3x4 v_9(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_10(uint start_byte_offset) {
  uint v_11 = v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)];
  uint v_12 = v.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)];
  mat3x4 v_13 = v_9((16u + start_byte_offset));
  tint_GammaTransferParams v_14 = v_8((64u + start_byte_offset));
  tint_GammaTransferParams v_15 = v_8((96u + start_byte_offset));
  mat3 v_16 = v_7((128u + start_byte_offset));
  mat3x2 v_17 = v_1((176u + start_byte_offset));
  mat3x2 v_18 = v_1((200u + start_byte_offset));
  uvec4 v_19 = v.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_20 = uintBitsToFloat(mix(v_19.xy, v_19.zw, bvec2(((((224u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_21 = v.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_22 = uintBitsToFloat(mix(v_21.xy, v_21.zw, bvec2(((((232u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_23 = v.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_24 = uintBitsToFloat(mix(v_23.xy, v_23.zw, bvec2(((((240u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_25 = v.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_26 = uintBitsToFloat(mix(v_25.xy, v_25.zw, bvec2(((((248u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_27 = v.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_28 = mix(v_27.xy, v_27.zw, bvec2(((((256u + start_byte_offset) % 16u) / 4u) == 2u)));
  uvec4 v_29 = v.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_11, v_12, v_13, v_14, v_15, v_16, v_17, v_18, v_20, v_22, v_24, v_26, v_28, uintBitsToFloat(mix(v_29.xy, v_29.zw, bvec2(((((264u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
uvec2 textureDimensions_cdc6c9() {
  uvec2 res = (v_10(0u).apparentSize + uvec2(1u));
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v_30 = VertexOutput(vec4(0.0f), uvec2(0u));
  v_30.pos = vec4(0.0f);
  v_30.prevent_dce = textureDimensions_cdc6c9();
  return v_30;
}
void main() {
  VertexOutput v_31 = vertex_main_inner();
  gl_Position = vec4(v_31.pos.x, -(v_31.pos.y), ((2.0f * v_31.pos.z) - v_31.pos.w), v_31.pos.w);
  tint_interstage_location0 = v_31.prevent_dce;
  gl_PointSize = 1.0f;
}
