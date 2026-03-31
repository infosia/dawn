//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;


struct tint_TransferFunctionParams {
  uint mode;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  float G;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_TransferFunctionParams srcTransferFunction;
  tint_TransferFunctionParams dstTransferFunction;
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
  vec4 inner;
} v_1;
layout(binding = 4, std140)
uniform f_arg_0_params_block_ubo {
  uvec4 inner[17];
} v_2;
uniform highp sampler2D f_arg_0_plane0_arg_1;
uniform highp sampler2D f_arg_0_plane1_arg_1;
vec3 tint_ApplyGammaTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_3 = vec3(params.G);
  return mix((sign(v) * (pow(((params.A * abs(v)) + params.B), v_3) + params.E)), (sign(v) * ((params.C * abs(v)) + params.F)), lessThan(abs(v), vec3(params.D)));
}
float tint_ApplyHLGSingleChannel(float v, tint_TransferFunctionParams params) {
  if ((v <= params.D)) {
    return ((v * v) / params.E);
  } else {
    return ((params.B + exp(((v - params.C) / params.A))) / params.F);
  }
  /* unreachable */
  return 0.0f;
}
vec3 tint_ApplyHLGTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  float v_4 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_5 = tint_ApplyHLGSingleChannel(v.y, params);
  return vec3(v_4, v_5, tint_ApplyHLGSingleChannel(v.z, params));
}
vec3 tint_ApplyPQTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_6 = vec3(params.C);
  vec3 v_7 = vec3(params.D);
  vec3 v_8 = vec3(params.E);
  vec3 v_9 = vec3(params.A);
  vec3 v_10 = pow(clamp(v, vec3(0.0f), vec3(1.0f)), (vec3(1.0f) / vec3(params.B)));
  return pow((max((v_10 - v_6), vec3(0.0f)) / (v_7 - (v_8 * v_10))), (vec3(1.0f) / v_9));
}
vec3 tint_ApplySrcTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  if ((params.mode == 0u)) {
    return tint_ApplyGammaTransferFunction(v, params);
  } else {
    if ((params.mode == 1u)) {
      return tint_ApplyHLGTransferFunction(v, params);
    } else {
      return tint_ApplyPQTransferFunction(v, params);
    }
    /* unreachable */
    return vec3(0.0f);
  }
  /* unreachable */
  return vec3(0.0f);
}
vec4 tint_TextureSampleClampToEdgeMultiplanarExternal(tint_ExternalTextureParams params, vec2 coords) {
  vec2 v_11 = (params.sampleTransform * vec3(coords, 1.0f));
  vec3 v_12 = vec3(0.0f);
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    vec4 v_14 = textureLod(f_arg_0_plane0_arg_1, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_12 = v_14.xyz;
    v_13 = v_14.w;
  } else {
    v_12 = (vec4(textureLod(f_arg_0_plane0_arg_1, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, textureLod(f_arg_0_plane1_arg_1, clamp(v_11, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f) * params.yuvToRgbConversionMatrix);
    v_13 = 1.0f;
  }
  vec3 v_15 = v_12;
  vec3 v_16 = vec3(0.0f);
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    v_16 = tint_ApplyGammaTransferFunction((params.gamutConversionMatrix * tint_ApplySrcTransferFunction(v_15, params.srcTransferFunction)), params.dstTransferFunction);
  } else {
    v_16 = v_15;
  }
  return vec4(v_16, v_13);
}
mat3x2 v_17(uint start_byte_offset) {
  uvec4 v_18 = v_2.inner[(start_byte_offset / 16u)];
  vec2 v_19 = uintBitsToFloat(mix(v_18.xy, v_18.zw, bvec2((((start_byte_offset & 15u) >> 2u) == 2u))));
  uvec4 v_20 = v_2.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_21 = uintBitsToFloat(mix(v_20.xy, v_20.zw, bvec2(((((8u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_22 = v_2.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_19, v_21, uintBitsToFloat(mix(v_22.xy, v_22.zw, bvec2(((((16u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
mat3 v_23(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_2.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_2.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_2.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_TransferFunctionParams v_24(uint start_byte_offset) {
  uvec4 v_25 = v_2.inner[(start_byte_offset / 16u)];
  uvec4 v_26 = v_2.inner[((4u + start_byte_offset) / 16u)];
  uvec4 v_27 = v_2.inner[((8u + start_byte_offset) / 16u)];
  uvec4 v_28 = v_2.inner[((12u + start_byte_offset) / 16u)];
  uvec4 v_29 = v_2.inner[((16u + start_byte_offset) / 16u)];
  uvec4 v_30 = v_2.inner[((20u + start_byte_offset) / 16u)];
  uvec4 v_31 = v_2.inner[((24u + start_byte_offset) / 16u)];
  uvec4 v_32 = v_2.inner[((28u + start_byte_offset) / 16u)];
  return tint_TransferFunctionParams(v_25[((start_byte_offset & 15u) >> 2u)], uintBitsToFloat(v_26[(((4u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_27[(((8u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_28[(((12u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_29[(((16u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_30[(((20u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_31[(((24u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_32[(((28u + start_byte_offset) & 15u) >> 2u)]));
}
mat3x4 v_33(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_2.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_2.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_2.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_34(uint start_byte_offset) {
  uvec4 v_35 = v_2.inner[(start_byte_offset / 16u)];
  uvec4 v_36 = v_2.inner[((4u + start_byte_offset) / 16u)];
  mat3x4 v_37 = v_33((16u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_24((64u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_24((96u + start_byte_offset));
  mat3 v_40 = v_23((128u + start_byte_offset));
  mat3x2 v_41 = v_17((176u + start_byte_offset));
  mat3x2 v_42 = v_17((200u + start_byte_offset));
  uvec4 v_43 = v_2.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_44 = uintBitsToFloat(mix(v_43.xy, v_43.zw, bvec2(((((224u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_45 = v_2.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_46 = uintBitsToFloat(mix(v_45.xy, v_45.zw, bvec2(((((232u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_47 = v_2.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_48 = uintBitsToFloat(mix(v_47.xy, v_47.zw, bvec2(((((240u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_49 = v_2.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_50 = uintBitsToFloat(mix(v_49.xy, v_49.zw, bvec2(((((248u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_51 = v_2.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_52 = mix(v_51.xy, v_51.zw, bvec2(((((256u + start_byte_offset) & 15u) >> 2u) == 2u)));
  uvec4 v_53 = v_2.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_35[((start_byte_offset & 15u) >> 2u)], v_36[(((4u + start_byte_offset) & 15u) >> 2u)], v_37, v_38, v_39, v_40, v_41, v_42, v_44, v_46, v_48, v_50, v_52, uintBitsToFloat(mix(v_53.xy, v_53.zw, bvec2(((((264u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
vec4 textureSampleBaseClampToEdge_7c04e6() {
  vec4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(v_34(0u), vec2(1.0f));
  return res;
}
void main() {
  v_1.inner = textureSampleBaseClampToEdge_7c04e6();
}
//
// compute_main
//
#version 310 es


struct tint_TransferFunctionParams {
  uint mode;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  float G;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_TransferFunctionParams srcTransferFunction;
  tint_TransferFunctionParams dstTransferFunction;
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
  vec4 inner;
} v_1;
layout(binding = 4, std140)
uniform arg_0_params_block_1_ubo {
  uvec4 inner[17];
} v_2;
uniform highp sampler2D arg_0_plane0_arg_1;
uniform highp sampler2D arg_0_plane1_arg_1;
vec3 tint_ApplyGammaTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_3 = vec3(params.G);
  return mix((sign(v) * (pow(((params.A * abs(v)) + params.B), v_3) + params.E)), (sign(v) * ((params.C * abs(v)) + params.F)), lessThan(abs(v), vec3(params.D)));
}
float tint_ApplyHLGSingleChannel(float v, tint_TransferFunctionParams params) {
  if ((v <= params.D)) {
    return ((v * v) / params.E);
  } else {
    return ((params.B + exp(((v - params.C) / params.A))) / params.F);
  }
  /* unreachable */
  return 0.0f;
}
vec3 tint_ApplyHLGTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  float v_4 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_5 = tint_ApplyHLGSingleChannel(v.y, params);
  return vec3(v_4, v_5, tint_ApplyHLGSingleChannel(v.z, params));
}
vec3 tint_ApplyPQTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_6 = vec3(params.C);
  vec3 v_7 = vec3(params.D);
  vec3 v_8 = vec3(params.E);
  vec3 v_9 = vec3(params.A);
  vec3 v_10 = pow(clamp(v, vec3(0.0f), vec3(1.0f)), (vec3(1.0f) / vec3(params.B)));
  return pow((max((v_10 - v_6), vec3(0.0f)) / (v_7 - (v_8 * v_10))), (vec3(1.0f) / v_9));
}
vec3 tint_ApplySrcTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  if ((params.mode == 0u)) {
    return tint_ApplyGammaTransferFunction(v, params);
  } else {
    if ((params.mode == 1u)) {
      return tint_ApplyHLGTransferFunction(v, params);
    } else {
      return tint_ApplyPQTransferFunction(v, params);
    }
    /* unreachable */
    return vec3(0.0f);
  }
  /* unreachable */
  return vec3(0.0f);
}
vec4 tint_TextureSampleClampToEdgeMultiplanarExternal(tint_ExternalTextureParams params, vec2 coords) {
  vec2 v_11 = (params.sampleTransform * vec3(coords, 1.0f));
  vec3 v_12 = vec3(0.0f);
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    vec4 v_14 = textureLod(arg_0_plane0_arg_1, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_12 = v_14.xyz;
    v_13 = v_14.w;
  } else {
    v_12 = (vec4(textureLod(arg_0_plane0_arg_1, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, textureLod(arg_0_plane1_arg_1, clamp(v_11, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f) * params.yuvToRgbConversionMatrix);
    v_13 = 1.0f;
  }
  vec3 v_15 = v_12;
  vec3 v_16 = vec3(0.0f);
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    v_16 = tint_ApplyGammaTransferFunction((params.gamutConversionMatrix * tint_ApplySrcTransferFunction(v_15, params.srcTransferFunction)), params.dstTransferFunction);
  } else {
    v_16 = v_15;
  }
  return vec4(v_16, v_13);
}
mat3x2 v_17(uint start_byte_offset) {
  uvec4 v_18 = v_2.inner[(start_byte_offset / 16u)];
  vec2 v_19 = uintBitsToFloat(mix(v_18.xy, v_18.zw, bvec2((((start_byte_offset & 15u) >> 2u) == 2u))));
  uvec4 v_20 = v_2.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_21 = uintBitsToFloat(mix(v_20.xy, v_20.zw, bvec2(((((8u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_22 = v_2.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_19, v_21, uintBitsToFloat(mix(v_22.xy, v_22.zw, bvec2(((((16u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
mat3 v_23(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_2.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_2.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_2.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_TransferFunctionParams v_24(uint start_byte_offset) {
  uvec4 v_25 = v_2.inner[(start_byte_offset / 16u)];
  uvec4 v_26 = v_2.inner[((4u + start_byte_offset) / 16u)];
  uvec4 v_27 = v_2.inner[((8u + start_byte_offset) / 16u)];
  uvec4 v_28 = v_2.inner[((12u + start_byte_offset) / 16u)];
  uvec4 v_29 = v_2.inner[((16u + start_byte_offset) / 16u)];
  uvec4 v_30 = v_2.inner[((20u + start_byte_offset) / 16u)];
  uvec4 v_31 = v_2.inner[((24u + start_byte_offset) / 16u)];
  uvec4 v_32 = v_2.inner[((28u + start_byte_offset) / 16u)];
  return tint_TransferFunctionParams(v_25[((start_byte_offset & 15u) >> 2u)], uintBitsToFloat(v_26[(((4u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_27[(((8u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_28[(((12u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_29[(((16u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_30[(((20u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_31[(((24u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_32[(((28u + start_byte_offset) & 15u) >> 2u)]));
}
mat3x4 v_33(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_2.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_2.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_2.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_34(uint start_byte_offset) {
  uvec4 v_35 = v_2.inner[(start_byte_offset / 16u)];
  uvec4 v_36 = v_2.inner[((4u + start_byte_offset) / 16u)];
  mat3x4 v_37 = v_33((16u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_24((64u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_24((96u + start_byte_offset));
  mat3 v_40 = v_23((128u + start_byte_offset));
  mat3x2 v_41 = v_17((176u + start_byte_offset));
  mat3x2 v_42 = v_17((200u + start_byte_offset));
  uvec4 v_43 = v_2.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_44 = uintBitsToFloat(mix(v_43.xy, v_43.zw, bvec2(((((224u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_45 = v_2.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_46 = uintBitsToFloat(mix(v_45.xy, v_45.zw, bvec2(((((232u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_47 = v_2.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_48 = uintBitsToFloat(mix(v_47.xy, v_47.zw, bvec2(((((240u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_49 = v_2.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_50 = uintBitsToFloat(mix(v_49.xy, v_49.zw, bvec2(((((248u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_51 = v_2.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_52 = mix(v_51.xy, v_51.zw, bvec2(((((256u + start_byte_offset) & 15u) >> 2u) == 2u)));
  uvec4 v_53 = v_2.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_35[((start_byte_offset & 15u) >> 2u)], v_36[(((4u + start_byte_offset) & 15u) >> 2u)], v_37, v_38, v_39, v_40, v_41, v_42, v_44, v_46, v_48, v_50, v_52, uintBitsToFloat(mix(v_53.xy, v_53.zw, bvec2(((((264u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
vec4 textureSampleBaseClampToEdge_7c04e6() {
  vec4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(v_34(0u), vec2(1.0f));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v_1.inner = textureSampleBaseClampToEdge_7c04e6();
}
//
// vertex_main
//
#version 310 es


struct tint_TransferFunctionParams {
  uint mode;
  float A;
  float B;
  float C;
  float D;
  float E;
  float F;
  float G;
};

struct tint_ExternalTextureParams {
  uint numPlanes;
  uint doYuvToRgbConversionOnly;
  mat3x4 yuvToRgbConversionMatrix;
  tint_TransferFunctionParams srcTransferFunction;
  tint_TransferFunctionParams dstTransferFunction;
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
  vec4 prevent_dce;
};

layout(binding = 3, std140)
uniform v_arg_0_params_block_ubo {
  uvec4 inner[17];
} v_1;
uniform highp sampler2D v_arg_0_plane0_arg_1;
uniform highp sampler2D v_arg_0_plane1_arg_1;
layout(location = 0) flat out vec4 tint_interstage_location0;
vec3 tint_ApplyGammaTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_2 = vec3(params.G);
  return mix((sign(v) * (pow(((params.A * abs(v)) + params.B), v_2) + params.E)), (sign(v) * ((params.C * abs(v)) + params.F)), lessThan(abs(v), vec3(params.D)));
}
float tint_ApplyHLGSingleChannel(float v, tint_TransferFunctionParams params) {
  if ((v <= params.D)) {
    return ((v * v) / params.E);
  } else {
    return ((params.B + exp(((v - params.C) / params.A))) / params.F);
  }
  /* unreachable */
  return 0.0f;
}
vec3 tint_ApplyHLGTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  float v_3 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_4 = tint_ApplyHLGSingleChannel(v.y, params);
  return vec3(v_3, v_4, tint_ApplyHLGSingleChannel(v.z, params));
}
vec3 tint_ApplyPQTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  vec3 v_5 = vec3(params.C);
  vec3 v_6 = vec3(params.D);
  vec3 v_7 = vec3(params.E);
  vec3 v_8 = vec3(params.A);
  vec3 v_9 = pow(clamp(v, vec3(0.0f), vec3(1.0f)), (vec3(1.0f) / vec3(params.B)));
  return pow((max((v_9 - v_5), vec3(0.0f)) / (v_6 - (v_7 * v_9))), (vec3(1.0f) / v_8));
}
vec3 tint_ApplySrcTransferFunction(vec3 v, tint_TransferFunctionParams params) {
  if ((params.mode == 0u)) {
    return tint_ApplyGammaTransferFunction(v, params);
  } else {
    if ((params.mode == 1u)) {
      return tint_ApplyHLGTransferFunction(v, params);
    } else {
      return tint_ApplyPQTransferFunction(v, params);
    }
    /* unreachable */
    return vec3(0.0f);
  }
  /* unreachable */
  return vec3(0.0f);
}
vec4 tint_TextureSampleClampToEdgeMultiplanarExternal(tint_ExternalTextureParams params, vec2 coords) {
  vec2 v_10 = (params.sampleTransform * vec3(coords, 1.0f));
  vec3 v_11 = vec3(0.0f);
  float v_12 = 0.0f;
  if ((params.numPlanes == 1u)) {
    vec4 v_13 = textureLod(v_arg_0_plane0_arg_1, clamp(v_10, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_11 = v_13.xyz;
    v_12 = v_13.w;
  } else {
    v_11 = (vec4(textureLod(v_arg_0_plane0_arg_1, clamp(v_10, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, textureLod(v_arg_0_plane1_arg_1, clamp(v_10, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f) * params.yuvToRgbConversionMatrix);
    v_12 = 1.0f;
  }
  vec3 v_14 = v_11;
  vec3 v_15 = vec3(0.0f);
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    v_15 = tint_ApplyGammaTransferFunction((params.gamutConversionMatrix * tint_ApplySrcTransferFunction(v_14, params.srcTransferFunction)), params.dstTransferFunction);
  } else {
    v_15 = v_14;
  }
  return vec4(v_15, v_12);
}
mat3x2 v_16(uint start_byte_offset) {
  uvec4 v_17 = v_1.inner[(start_byte_offset / 16u)];
  vec2 v_18 = uintBitsToFloat(mix(v_17.xy, v_17.zw, bvec2((((start_byte_offset & 15u) >> 2u) == 2u))));
  uvec4 v_19 = v_1.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_20 = uintBitsToFloat(mix(v_19.xy, v_19.zw, bvec2(((((8u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_21 = v_1.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_18, v_20, uintBitsToFloat(mix(v_21.xy, v_21.zw, bvec2(((((16u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
mat3 v_22(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_TransferFunctionParams v_23(uint start_byte_offset) {
  uvec4 v_24 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_25 = v_1.inner[((4u + start_byte_offset) / 16u)];
  uvec4 v_26 = v_1.inner[((8u + start_byte_offset) / 16u)];
  uvec4 v_27 = v_1.inner[((12u + start_byte_offset) / 16u)];
  uvec4 v_28 = v_1.inner[((16u + start_byte_offset) / 16u)];
  uvec4 v_29 = v_1.inner[((20u + start_byte_offset) / 16u)];
  uvec4 v_30 = v_1.inner[((24u + start_byte_offset) / 16u)];
  uvec4 v_31 = v_1.inner[((28u + start_byte_offset) / 16u)];
  return tint_TransferFunctionParams(v_24[((start_byte_offset & 15u) >> 2u)], uintBitsToFloat(v_25[(((4u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_26[(((8u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_27[(((12u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_28[(((16u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_29[(((20u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_30[(((24u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_31[(((28u + start_byte_offset) & 15u) >> 2u)]));
}
mat3x4 v_32(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_33(uint start_byte_offset) {
  uvec4 v_34 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_35 = v_1.inner[((4u + start_byte_offset) / 16u)];
  mat3x4 v_36 = v_32((16u + start_byte_offset));
  tint_TransferFunctionParams v_37 = v_23((64u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_23((96u + start_byte_offset));
  mat3 v_39 = v_22((128u + start_byte_offset));
  mat3x2 v_40 = v_16((176u + start_byte_offset));
  mat3x2 v_41 = v_16((200u + start_byte_offset));
  uvec4 v_42 = v_1.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_43 = uintBitsToFloat(mix(v_42.xy, v_42.zw, bvec2(((((224u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_44 = v_1.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_45 = uintBitsToFloat(mix(v_44.xy, v_44.zw, bvec2(((((232u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_46 = v_1.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_47 = uintBitsToFloat(mix(v_46.xy, v_46.zw, bvec2(((((240u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_48 = v_1.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_49 = uintBitsToFloat(mix(v_48.xy, v_48.zw, bvec2(((((248u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_50 = v_1.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_51 = mix(v_50.xy, v_50.zw, bvec2(((((256u + start_byte_offset) & 15u) >> 2u) == 2u)));
  uvec4 v_52 = v_1.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_34[((start_byte_offset & 15u) >> 2u)], v_35[(((4u + start_byte_offset) & 15u) >> 2u)], v_36, v_37, v_38, v_39, v_40, v_41, v_43, v_45, v_47, v_49, v_51, uintBitsToFloat(mix(v_52.xy, v_52.zw, bvec2(((((264u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
vec4 textureSampleBaseClampToEdge_7c04e6() {
  vec4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(v_33(0u), vec2(1.0f));
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v_53 = VertexOutput(vec4(0.0f), vec4(0.0f));
  v_53.pos = vec4(0.0f);
  v_53.prevent_dce = textureSampleBaseClampToEdge_7c04e6();
  return v_53;
}
void main() {
  VertexOutput v_54 = vertex_main_inner();
  gl_Position = vec4(v_54.pos.x, -(v_54.pos.y), ((2.0f * v_54.pos.z) - v_54.pos.w), v_54.pos.w);
  tint_interstage_location0 = v_54.prevent_dce;
  gl_PointSize = 1.0f;
}
