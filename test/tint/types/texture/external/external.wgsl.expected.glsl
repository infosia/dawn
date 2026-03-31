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

layout(binding = 3, std140)
uniform t_f_params_block_1_ubo {
  uvec4 inner[17];
} v_1;
layout(binding = 0, std430)
buffer out_block_1_ssbo {
  vec4 inner;
} v_2;
uniform highp sampler2D t_f_plane0;
uniform highp sampler2D t_f_plane1;
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
vec4 tint_TextureLoadMultiplanarExternal(tint_ExternalTextureParams params, uvec2 coords) {
  vec2 v_11 = round((params.loadTransform * vec3(vec2(min(coords, params.apparentSize)), 1.0f)));
  uvec2 v_12 = uvec2(v_11);
  vec3 v_13 = vec3(0.0f);
  float v_14 = 0.0f;
  if ((params.numPlanes == 1u)) {
    ivec2 v_15 = ivec2(v_12);
    vec4 v_16 = texelFetch(t_f_plane0, v_15, int(0u));
    v_13 = v_16.xyz;
    v_14 = v_16.w;
  } else {
    ivec2 v_17 = ivec2(v_12);
    float v_18 = texelFetch(t_f_plane0, v_17, int(0u)).x;
    ivec2 v_19 = ivec2(uvec2((v_11 * params.plane1CoordFactor)));
    v_13 = (vec4(v_18, texelFetch(t_f_plane1, v_19, int(0u)).xy, 1.0f) * params.yuvToRgbConversionMatrix);
    v_14 = 1.0f;
  }
  vec3 v_20 = v_13;
  vec3 v_21 = vec3(0.0f);
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    v_21 = tint_ApplyGammaTransferFunction((params.gamutConversionMatrix * tint_ApplySrcTransferFunction(v_20, params.srcTransferFunction)), params.dstTransferFunction);
  } else {
    v_21 = v_20;
  }
  return vec4(v_21, v_14);
}
mat3x2 v_22(uint start_byte_offset) {
  uvec4 v_23 = v_1.inner[(start_byte_offset / 16u)];
  vec2 v_24 = uintBitsToFloat(mix(v_23.xy, v_23.zw, bvec2((((start_byte_offset & 15u) >> 2u) == 2u))));
  uvec4 v_25 = v_1.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_26 = uintBitsToFloat(mix(v_25.xy, v_25.zw, bvec2(((((8u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_27 = v_1.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_24, v_26, uintBitsToFloat(mix(v_27.xy, v_27.zw, bvec2(((((16u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
mat3 v_28(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_TransferFunctionParams v_29(uint start_byte_offset) {
  uvec4 v_30 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_31 = v_1.inner[((4u + start_byte_offset) / 16u)];
  uvec4 v_32 = v_1.inner[((8u + start_byte_offset) / 16u)];
  uvec4 v_33 = v_1.inner[((12u + start_byte_offset) / 16u)];
  uvec4 v_34 = v_1.inner[((16u + start_byte_offset) / 16u)];
  uvec4 v_35 = v_1.inner[((20u + start_byte_offset) / 16u)];
  uvec4 v_36 = v_1.inner[((24u + start_byte_offset) / 16u)];
  uvec4 v_37 = v_1.inner[((28u + start_byte_offset) / 16u)];
  return tint_TransferFunctionParams(v_30[((start_byte_offset & 15u) >> 2u)], uintBitsToFloat(v_31[(((4u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_32[(((8u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_33[(((12u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_34[(((16u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_35[(((20u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_36[(((24u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_37[(((28u + start_byte_offset) & 15u) >> 2u)]));
}
mat3x4 v_38(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_39(uint start_byte_offset) {
  uvec4 v_40 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_41 = v_1.inner[((4u + start_byte_offset) / 16u)];
  mat3x4 v_42 = v_38((16u + start_byte_offset));
  tint_TransferFunctionParams v_43 = v_29((64u + start_byte_offset));
  tint_TransferFunctionParams v_44 = v_29((96u + start_byte_offset));
  mat3 v_45 = v_28((128u + start_byte_offset));
  mat3x2 v_46 = v_22((176u + start_byte_offset));
  mat3x2 v_47 = v_22((200u + start_byte_offset));
  uvec4 v_48 = v_1.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_49 = uintBitsToFloat(mix(v_48.xy, v_48.zw, bvec2(((((224u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_50 = v_1.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_51 = uintBitsToFloat(mix(v_50.xy, v_50.zw, bvec2(((((232u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_52 = v_1.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_53 = uintBitsToFloat(mix(v_52.xy, v_52.zw, bvec2(((((240u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_54 = v_1.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_55 = uintBitsToFloat(mix(v_54.xy, v_54.zw, bvec2(((((248u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_56 = v_1.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_57 = mix(v_56.xy, v_56.zw, bvec2(((((256u + start_byte_offset) & 15u) >> 2u) == 2u)));
  uvec4 v_58 = v_1.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_40[((start_byte_offset & 15u) >> 2u)], v_41[(((4u + start_byte_offset) & 15u) >> 2u)], v_42, v_43, v_44, v_45, v_46, v_47, v_49, v_51, v_53, v_55, v_57, uintBitsToFloat(mix(v_58.xy, v_58.zw, bvec2(((((264u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_ExternalTextureParams v_59 = v_39(0u);
  v_2.inner = tint_TextureLoadMultiplanarExternal(v_59, min(uvec2(ivec2(0)), ((v_59.apparentSize + uvec2(1u)) - uvec2(1u))));
}
