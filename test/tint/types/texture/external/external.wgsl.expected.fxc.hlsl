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
  float3x4 yuvToRgbConversionMatrix;
  tint_TransferFunctionParams srcTransferFunction;
  tint_TransferFunctionParams dstTransferFunction;
  float3x3 gamutConversionMatrix;
  float3x2 sampleTransform;
  float3x2 loadTransform;
  float2 samplePlane0RectMin;
  float2 samplePlane0RectMax;
  float2 samplePlane1RectMin;
  float2 samplePlane1RectMax;
  uint2 apparentSize;
  float2 plane1CoordFactor;
};


cbuffer cbuffer_t_f_params : register(b2) {
  uint4 t_f_params[17];
};
Texture2D<float4> t_f_plane0 : register(t0);
Texture2D<float4> t_f_plane1 : register(t1);
RWByteAddressBuffer v_1 : register(u0, space1);
uint2 tint_v2f32_to_v2u32(float2 value) {
  return uint2(clamp(value, (0.0f).xx, (4294967040.0f).xx));
}

float3 tint_ApplyGammaTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_2 = float3((params.G).xxx);
  float3 v_3 = float3((params.D).xxx);
  float3 v_4 = float3(sign(v));
  return (((abs(v) < v_3)) ? ((v_4 * ((params.C * abs(v)) + params.F))) : ((v_4 * (pow(((params.A * abs(v)) + params.B), v_2) + params.E))));
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

float3 tint_ApplyHLGTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float v_5 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_6 = tint_ApplyHLGSingleChannel(v.y, params);
  return float3(v_5, v_6, tint_ApplyHLGSingleChannel(v.z, params));
}

float3 tint_ApplyPQTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_7 = float3((params.C).xxx);
  float3 v_8 = float3((params.D).xxx);
  float3 v_9 = float3((params.E).xxx);
  float3 v_10 = float3((params.A).xxx);
  float3 v_11 = pow(clamp(v, (0.0f).xxx, (1.0f).xxx), ((1.0f).xxx / float3((params.B).xxx)));
  return pow((max((v_11 - v_7), (0.0f).xxx) / (v_8 - (v_9 * v_11))), ((1.0f).xxx / v_10));
}

float3 tint_ApplySrcTransferFunction(float3 v, tint_TransferFunctionParams params) {
  if ((params.mode == 0u)) {
    return tint_ApplyGammaTransferFunction(v, params);
  } else {
    if ((params.mode == 1u)) {
      return tint_ApplyHLGTransferFunction(v, params);
    } else {
      return tint_ApplyPQTransferFunction(v, params);
    }
    /* unreachable */
    return (0.0f).xxx;
  }
  /* unreachable */
  return (0.0f).xxx;
}

float4 tint_TextureLoadMultiplanarExternal(Texture2D<float4> plane_0, Texture2D<float4> plane_1, tint_ExternalTextureParams params, uint2 coords) {
  float2 v_12 = round(mul(float3(float2(min(coords, params.apparentSize)), 1.0f), params.loadTransform));
  uint2 v_13 = tint_v2f32_to_v2u32(v_12);
  float3 v_14 = (0.0f).xxx;
  float v_15 = 0.0f;
  if ((params.numPlanes == 1u)) {
    int2 v_16 = int2(v_13);
    float4 v_17 = plane_0.Load(int3(v_16, int(0u)));
    v_14 = v_17.xyz;
    v_15 = v_17.w;
  } else {
    int2 v_18 = int2(v_13);
    float v_19 = plane_0.Load(int3(v_18, int(0u))).x;
    int2 v_20 = int2(tint_v2f32_to_v2u32((v_12 * params.plane1CoordFactor)));
    v_14 = mul(params.yuvToRgbConversionMatrix, float4(v_19, plane_1.Load(int3(v_20, int(0u))).xy, 1.0f));
    v_15 = 1.0f;
  }
  float3 v_21 = v_14;
  float3 v_22 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_23 = params.srcTransferFunction;
    tint_TransferFunctionParams v_24 = params.dstTransferFunction;
    v_22 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_21, v_23), params.gamutConversionMatrix), v_24);
  } else {
    v_22 = v_21;
  }
  return float4(v_22, v_15);
}

float3x2 v_25(uint start_byte_offset) {
  uint4 v_26 = t_f_params[(start_byte_offset / 16u)];
  float2 v_27 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_26.zw) : (v_26.xy)));
  uint4 v_28 = t_f_params[((8u + start_byte_offset) / 16u)];
  float2 v_29 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_28.zw) : (v_28.xy)));
  uint4 v_30 = t_f_params[((16u + start_byte_offset) / 16u)];
  return float3x2(v_27, v_29, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_30.zw) : (v_30.xy))));
}

float3x3 v_31(uint start_byte_offset) {
  return float3x3(asfloat(t_f_params[(start_byte_offset / 16u)].xyz), asfloat(t_f_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(t_f_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_32(uint start_byte_offset) {
  tint_TransferFunctionParams v_33 = {t_f_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(t_f_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(t_f_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_33;
}

float3x4 v_34(uint start_byte_offset) {
  return float3x4(asfloat(t_f_params[(start_byte_offset / 16u)]), asfloat(t_f_params[((16u + start_byte_offset) / 16u)]), asfloat(t_f_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_35(uint start_byte_offset) {
  uint v_36 = t_f_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_37 = t_f_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_38 = v_34((16u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_32((64u + start_byte_offset));
  tint_TransferFunctionParams v_40 = v_32((96u + start_byte_offset));
  float3x3 v_41 = v_31((128u + start_byte_offset));
  float3x2 v_42 = v_25((176u + start_byte_offset));
  float3x2 v_43 = v_25((200u + start_byte_offset));
  uint4 v_44 = t_f_params[((224u + start_byte_offset) / 16u)];
  float2 v_45 = asfloat(((((((224u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_44.zw) : (v_44.xy)));
  uint4 v_46 = t_f_params[((232u + start_byte_offset) / 16u)];
  float2 v_47 = asfloat(((((((232u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_46.zw) : (v_46.xy)));
  uint4 v_48 = t_f_params[((240u + start_byte_offset) / 16u)];
  float2 v_49 = asfloat(((((((240u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_48.zw) : (v_48.xy)));
  uint4 v_50 = t_f_params[((248u + start_byte_offset) / 16u)];
  float2 v_51 = asfloat(((((((248u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_50.zw) : (v_50.xy)));
  uint4 v_52 = t_f_params[((256u + start_byte_offset) / 16u)];
  uint2 v_53 = ((((((256u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_52.zw) : (v_52.xy));
  uint4 v_54 = t_f_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_55 = {v_36, v_37, v_38, v_39, v_40, v_41, v_42, v_43, v_45, v_47, v_49, v_51, v_53, asfloat(((((((264u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_54.zw) : (v_54.xy)))};
  return v_55;
}

[numthreads(1, 1, 1)]
void main() {
  tint_ExternalTextureParams v_56 = v_35(0u);
  v_1.Store4(0u, asuint(tint_TextureLoadMultiplanarExternal(t_f_plane0, t_f_plane1, v_56, uint2((int(0)).xx))));
}

