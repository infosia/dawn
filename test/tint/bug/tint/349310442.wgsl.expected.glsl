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

layout(binding = 2, std140)
uniform t_params_block_1_ubo {
  uvec4 inner[17];
} v_1;
uniform highp sampler2D t_plane0;
uniform highp sampler2D t_plane1;
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
vec4 tint_TextureLoadMultiplanarExternal(tint_ExternalTextureParams params, uvec2 coords) {
  vec2 v_10 = round((params.loadTransform * vec3(vec2(min(coords, params.apparentSize)), 1.0f)));
  uvec2 v_11 = uvec2(v_10);
  vec3 v_12 = vec3(0.0f);
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    ivec2 v_14 = ivec2(v_11);
    vec4 v_15 = texelFetch(t_plane0, v_14, int(0u));
    v_12 = v_15.xyz;
    v_13 = v_15.w;
  } else {
    ivec2 v_16 = ivec2(v_11);
    float v_17 = texelFetch(t_plane0, v_16, int(0u)).x;
    ivec2 v_18 = ivec2(uvec2((v_10 * params.plane1CoordFactor)));
    v_12 = (vec4(v_17, texelFetch(t_plane1, v_18, int(0u)).xy, 1.0f) * params.yuvToRgbConversionMatrix);
    v_13 = 1.0f;
  }
  vec3 v_19 = v_12;
  vec3 v_20 = vec3(0.0f);
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    v_20 = tint_ApplyGammaTransferFunction((params.gamutConversionMatrix * tint_ApplySrcTransferFunction(v_19, params.srcTransferFunction)), params.dstTransferFunction);
  } else {
    v_20 = v_19;
  }
  return vec4(v_20, v_13);
}
mat3x2 v_21(uint start_byte_offset) {
  uvec4 v_22 = v_1.inner[(start_byte_offset / 16u)];
  vec2 v_23 = uintBitsToFloat(mix(v_22.xy, v_22.zw, bvec2((((start_byte_offset & 15u) >> 2u) == 2u))));
  uvec4 v_24 = v_1.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_25 = uintBitsToFloat(mix(v_24.xy, v_24.zw, bvec2(((((8u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_26 = v_1.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_23, v_25, uintBitsToFloat(mix(v_26.xy, v_26.zw, bvec2(((((16u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
mat3 v_27(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)].xyz));
}
tint_TransferFunctionParams v_28(uint start_byte_offset) {
  uvec4 v_29 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_30 = v_1.inner[((4u + start_byte_offset) / 16u)];
  uvec4 v_31 = v_1.inner[((8u + start_byte_offset) / 16u)];
  uvec4 v_32 = v_1.inner[((12u + start_byte_offset) / 16u)];
  uvec4 v_33 = v_1.inner[((16u + start_byte_offset) / 16u)];
  uvec4 v_34 = v_1.inner[((20u + start_byte_offset) / 16u)];
  uvec4 v_35 = v_1.inner[((24u + start_byte_offset) / 16u)];
  uvec4 v_36 = v_1.inner[((28u + start_byte_offset) / 16u)];
  return tint_TransferFunctionParams(v_29[((start_byte_offset & 15u) >> 2u)], uintBitsToFloat(v_30[(((4u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_31[(((8u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_32[(((12u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_33[(((16u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_34[(((20u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_35[(((24u + start_byte_offset) & 15u) >> 2u)]), uintBitsToFloat(v_36[(((28u + start_byte_offset) & 15u) >> 2u)]));
}
mat3x4 v_37(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]));
}
tint_ExternalTextureParams v_38(uint start_byte_offset) {
  uvec4 v_39 = v_1.inner[(start_byte_offset / 16u)];
  uvec4 v_40 = v_1.inner[((4u + start_byte_offset) / 16u)];
  mat3x4 v_41 = v_37((16u + start_byte_offset));
  tint_TransferFunctionParams v_42 = v_28((64u + start_byte_offset));
  tint_TransferFunctionParams v_43 = v_28((96u + start_byte_offset));
  mat3 v_44 = v_27((128u + start_byte_offset));
  mat3x2 v_45 = v_21((176u + start_byte_offset));
  mat3x2 v_46 = v_21((200u + start_byte_offset));
  uvec4 v_47 = v_1.inner[((224u + start_byte_offset) / 16u)];
  vec2 v_48 = uintBitsToFloat(mix(v_47.xy, v_47.zw, bvec2(((((224u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_49 = v_1.inner[((232u + start_byte_offset) / 16u)];
  vec2 v_50 = uintBitsToFloat(mix(v_49.xy, v_49.zw, bvec2(((((232u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_51 = v_1.inner[((240u + start_byte_offset) / 16u)];
  vec2 v_52 = uintBitsToFloat(mix(v_51.xy, v_51.zw, bvec2(((((240u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_53 = v_1.inner[((248u + start_byte_offset) / 16u)];
  vec2 v_54 = uintBitsToFloat(mix(v_53.xy, v_53.zw, bvec2(((((248u + start_byte_offset) & 15u) >> 2u) == 2u))));
  uvec4 v_55 = v_1.inner[((256u + start_byte_offset) / 16u)];
  uvec2 v_56 = mix(v_55.xy, v_55.zw, bvec2(((((256u + start_byte_offset) & 15u) >> 2u) == 2u)));
  uvec4 v_57 = v_1.inner[((264u + start_byte_offset) / 16u)];
  return tint_ExternalTextureParams(v_39[((start_byte_offset & 15u) >> 2u)], v_40[(((4u + start_byte_offset) & 15u) >> 2u)], v_41, v_42, v_43, v_44, v_45, v_46, v_48, v_50, v_52, v_54, v_56, uintBitsToFloat(mix(v_57.xy, v_57.zw, bvec2(((((264u + start_byte_offset) & 15u) >> 2u) == 2u)))));
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_ExternalTextureParams v_58 = v_38(0u);
  vec4 r = tint_TextureLoadMultiplanarExternal(v_58, min(uvec2(ivec2(0)), ((v_58.apparentSize + uvec2(1u)) - uvec2(1u))));
}
