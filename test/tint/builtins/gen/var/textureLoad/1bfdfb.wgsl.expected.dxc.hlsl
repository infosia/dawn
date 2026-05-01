//
// fragment_main
//
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


RWByteAddressBuffer prevent_dce : register(u0);
cbuffer cbuffer_arg_0_params : register(b2, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t1, space1);
uint2 tint_v2f32_to_v2u32(float2 value) {
  return uint2(clamp(value, (0.0f).xx, (4294967040.0f).xx));
}

float3 tint_ApplyGammaTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_1 = float3((params.G).xxx);
  float3 v_2 = float3((params.D).xxx);
  float3 v_3 = float3(sign(v));
  return select((abs(v) < v_2), (v_3 * ((params.C * abs(v)) + params.F)), (v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E)));
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
  float v_4 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_5 = tint_ApplyHLGSingleChannel(v.y, params);
  return float3(v_4, v_5, tint_ApplyHLGSingleChannel(v.z, params));
}

float3 tint_ApplyPQTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_6 = float3((params.C).xxx);
  float3 v_7 = float3((params.D).xxx);
  float3 v_8 = float3((params.E).xxx);
  float3 v_9 = float3((params.A).xxx);
  float3 v_10 = pow(clamp(v, (0.0f).xxx, (1.0f).xxx), ((1.0f).xxx / float3((params.B).xxx)));
  return pow((max((v_10 - v_6), (0.0f).xxx) / (v_7 - (v_8 * v_10))), ((1.0f).xxx / v_9));
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
  float2 v_11 = round(mul(float3(float2(min(coords, params.apparentSize)), 1.0f), params.loadTransform));
  uint2 v_12 = tint_v2f32_to_v2u32(v_11);
  float3 v_13 = (0.0f).xxx;
  float v_14 = 0.0f;
  if ((params.numPlanes == 1u)) {
    int2 v_15 = int2(v_12);
    float4 v_16 = plane_0.Load(int3(v_15, int(0u)));
    v_13 = v_16.xyz;
    v_14 = v_16.w;
  } else {
    int2 v_17 = int2(v_12);
    float v_18 = plane_0.Load(int3(v_17, int(0u))).x;
    int2 v_19 = int2(tint_v2f32_to_v2u32((v_11 * params.plane1CoordFactor)));
    v_13 = mul(params.yuvToRgbConversionMatrix, float4(v_18, plane_1.Load(int3(v_19, int(0u))).xy, 1.0f));
    v_14 = 1.0f;
  }
  float3 v_20 = v_13;
  float3 v_21 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_22 = params.srcTransferFunction;
    tint_TransferFunctionParams v_23 = params.dstTransferFunction;
    v_21 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_20, v_22), params.gamutConversionMatrix), v_23);
  } else {
    v_21 = v_20;
  }
  return float4(v_21, v_14);
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_26 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_27 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_25.zw, v_25.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_26.zw, v_26.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_27.zw, v_27.xy)));
}

float3x3 v_28(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_29(uint start_byte_offset) {
  tint_TransferFunctionParams v_30 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_30;
}

float3x4 v_31(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_32(uint start_byte_offset) {
  uint v_33 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_34 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_35 = v_31((16u + start_byte_offset));
  tint_TransferFunctionParams v_36 = v_29((64u + start_byte_offset));
  tint_TransferFunctionParams v_37 = v_29((96u + start_byte_offset));
  float3x3 v_38 = v_28((128u + start_byte_offset));
  float3x2 v_39 = v_24((176u + start_byte_offset));
  float3x2 v_40 = v_24((200u + start_byte_offset));
  uint4 v_41 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_42 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_43 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_44 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_45 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_46 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_47 = {v_33, v_34, v_35, v_36, v_37, v_38, v_39, v_40, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_42.zw, v_42.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_43.zw, v_43.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_44.zw, v_44.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_45.zw, v_45.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_46.zw, v_46.xy))};
  return v_47;
}

float4 textureLoad_1bfdfb() {
  uint2 arg_1 = (1u).xx;
  tint_ExternalTextureParams v_48 = v_32(0u);
  float4 res = tint_TextureLoadMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_48, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(textureLoad_1bfdfb()));
}

//
// compute_main
//
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


RWByteAddressBuffer prevent_dce : register(u0);
cbuffer cbuffer_arg_0_params : register(b2, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t1, space1);
uint2 tint_v2f32_to_v2u32(float2 value) {
  return uint2(clamp(value, (0.0f).xx, (4294967040.0f).xx));
}

float3 tint_ApplyGammaTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_1 = float3((params.G).xxx);
  float3 v_2 = float3((params.D).xxx);
  float3 v_3 = float3(sign(v));
  return select((abs(v) < v_2), (v_3 * ((params.C * abs(v)) + params.F)), (v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E)));
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
  float v_4 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_5 = tint_ApplyHLGSingleChannel(v.y, params);
  return float3(v_4, v_5, tint_ApplyHLGSingleChannel(v.z, params));
}

float3 tint_ApplyPQTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_6 = float3((params.C).xxx);
  float3 v_7 = float3((params.D).xxx);
  float3 v_8 = float3((params.E).xxx);
  float3 v_9 = float3((params.A).xxx);
  float3 v_10 = pow(clamp(v, (0.0f).xxx, (1.0f).xxx), ((1.0f).xxx / float3((params.B).xxx)));
  return pow((max((v_10 - v_6), (0.0f).xxx) / (v_7 - (v_8 * v_10))), ((1.0f).xxx / v_9));
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
  float2 v_11 = round(mul(float3(float2(min(coords, params.apparentSize)), 1.0f), params.loadTransform));
  uint2 v_12 = tint_v2f32_to_v2u32(v_11);
  float3 v_13 = (0.0f).xxx;
  float v_14 = 0.0f;
  if ((params.numPlanes == 1u)) {
    int2 v_15 = int2(v_12);
    float4 v_16 = plane_0.Load(int3(v_15, int(0u)));
    v_13 = v_16.xyz;
    v_14 = v_16.w;
  } else {
    int2 v_17 = int2(v_12);
    float v_18 = plane_0.Load(int3(v_17, int(0u))).x;
    int2 v_19 = int2(tint_v2f32_to_v2u32((v_11 * params.plane1CoordFactor)));
    v_13 = mul(params.yuvToRgbConversionMatrix, float4(v_18, plane_1.Load(int3(v_19, int(0u))).xy, 1.0f));
    v_14 = 1.0f;
  }
  float3 v_20 = v_13;
  float3 v_21 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_22 = params.srcTransferFunction;
    tint_TransferFunctionParams v_23 = params.dstTransferFunction;
    v_21 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_20, v_22), params.gamutConversionMatrix), v_23);
  } else {
    v_21 = v_20;
  }
  return float4(v_21, v_14);
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_26 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_27 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_25.zw, v_25.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_26.zw, v_26.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_27.zw, v_27.xy)));
}

float3x3 v_28(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_29(uint start_byte_offset) {
  tint_TransferFunctionParams v_30 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_30;
}

float3x4 v_31(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_32(uint start_byte_offset) {
  uint v_33 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_34 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_35 = v_31((16u + start_byte_offset));
  tint_TransferFunctionParams v_36 = v_29((64u + start_byte_offset));
  tint_TransferFunctionParams v_37 = v_29((96u + start_byte_offset));
  float3x3 v_38 = v_28((128u + start_byte_offset));
  float3x2 v_39 = v_24((176u + start_byte_offset));
  float3x2 v_40 = v_24((200u + start_byte_offset));
  uint4 v_41 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_42 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_43 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_44 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_45 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_46 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_47 = {v_33, v_34, v_35, v_36, v_37, v_38, v_39, v_40, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_42.zw, v_42.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_43.zw, v_43.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_44.zw, v_44.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_45.zw, v_45.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_46.zw, v_46.xy))};
  return v_47;
}

float4 textureLoad_1bfdfb() {
  uint2 arg_1 = (1u).xx;
  tint_ExternalTextureParams v_48 = v_32(0u);
  float4 res = tint_TextureLoadMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_48, arg_1);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(textureLoad_1bfdfb()));
}

//
// vertex_main
//
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

struct VertexOutput {
  float4 pos;
  float4 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation float4 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


cbuffer cbuffer_arg_0_params : register(b2, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t1, space1);
uint2 tint_v2f32_to_v2u32(float2 value) {
  return uint2(clamp(value, (0.0f).xx, (4294967040.0f).xx));
}

float3 tint_ApplyGammaTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_1 = float3((params.G).xxx);
  float3 v_2 = float3((params.D).xxx);
  float3 v_3 = float3(sign(v));
  return select((abs(v) < v_2), (v_3 * ((params.C * abs(v)) + params.F)), (v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E)));
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
  float v_4 = tint_ApplyHLGSingleChannel(v.x, params);
  float v_5 = tint_ApplyHLGSingleChannel(v.y, params);
  return float3(v_4, v_5, tint_ApplyHLGSingleChannel(v.z, params));
}

float3 tint_ApplyPQTransferFunction(float3 v, tint_TransferFunctionParams params) {
  float3 v_6 = float3((params.C).xxx);
  float3 v_7 = float3((params.D).xxx);
  float3 v_8 = float3((params.E).xxx);
  float3 v_9 = float3((params.A).xxx);
  float3 v_10 = pow(clamp(v, (0.0f).xxx, (1.0f).xxx), ((1.0f).xxx / float3((params.B).xxx)));
  return pow((max((v_10 - v_6), (0.0f).xxx) / (v_7 - (v_8 * v_10))), ((1.0f).xxx / v_9));
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
  float2 v_11 = round(mul(float3(float2(min(coords, params.apparentSize)), 1.0f), params.loadTransform));
  uint2 v_12 = tint_v2f32_to_v2u32(v_11);
  float3 v_13 = (0.0f).xxx;
  float v_14 = 0.0f;
  if ((params.numPlanes == 1u)) {
    int2 v_15 = int2(v_12);
    float4 v_16 = plane_0.Load(int3(v_15, int(0u)));
    v_13 = v_16.xyz;
    v_14 = v_16.w;
  } else {
    int2 v_17 = int2(v_12);
    float v_18 = plane_0.Load(int3(v_17, int(0u))).x;
    int2 v_19 = int2(tint_v2f32_to_v2u32((v_11 * params.plane1CoordFactor)));
    v_13 = mul(params.yuvToRgbConversionMatrix, float4(v_18, plane_1.Load(int3(v_19, int(0u))).xy, 1.0f));
    v_14 = 1.0f;
  }
  float3 v_20 = v_13;
  float3 v_21 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_22 = params.srcTransferFunction;
    tint_TransferFunctionParams v_23 = params.dstTransferFunction;
    v_21 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_20, v_22), params.gamutConversionMatrix), v_23);
  } else {
    v_21 = v_20;
  }
  return float4(v_21, v_14);
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_26 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_27 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_25.zw, v_25.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_26.zw, v_26.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_27.zw, v_27.xy)));
}

float3x3 v_28(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_29(uint start_byte_offset) {
  tint_TransferFunctionParams v_30 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_30;
}

float3x4 v_31(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_32(uint start_byte_offset) {
  uint v_33 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_34 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_35 = v_31((16u + start_byte_offset));
  tint_TransferFunctionParams v_36 = v_29((64u + start_byte_offset));
  tint_TransferFunctionParams v_37 = v_29((96u + start_byte_offset));
  float3x3 v_38 = v_28((128u + start_byte_offset));
  float3x2 v_39 = v_24((176u + start_byte_offset));
  float3x2 v_40 = v_24((200u + start_byte_offset));
  uint4 v_41 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_42 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_43 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_44 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_45 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_46 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_47 = {v_33, v_34, v_35, v_36, v_37, v_38, v_39, v_40, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_42.zw, v_42.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_43.zw, v_43.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_44.zw, v_44.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_45.zw, v_45.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_46.zw, v_46.xy))};
  return v_47;
}

float4 textureLoad_1bfdfb() {
  uint2 arg_1 = (1u).xx;
  tint_ExternalTextureParams v_48 = v_32(0u);
  float4 res = tint_TextureLoadMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_48, arg_1);
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_49 = (VertexOutput)0;
  v_49.pos = (0.0f).xxxx;
  v_49.prevent_dce = textureLoad_1bfdfb();
  VertexOutput v_50 = v_49;
  return v_50;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_51 = vertex_main_inner();
  vertex_main_outputs v_52 = {v_51.prevent_dce, v_51.pos};
  return v_52;
}

