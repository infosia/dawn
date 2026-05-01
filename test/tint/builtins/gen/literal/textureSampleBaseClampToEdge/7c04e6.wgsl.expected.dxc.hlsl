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
cbuffer cbuffer_arg_0_params : register(b3, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t2, space1);
SamplerState arg_1 : register(s1, space1);
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

float4 tint_TextureSampleClampToEdgeMultiplanarExternal(Texture2D<float4> plane_0, Texture2D<float4> plane_1, tint_ExternalTextureParams params, SamplerState tint_sampler, float2 coords) {
  float2 v_11 = mul(float3(coords, 1.0f), params.sampleTransform);
  float3 v_12 = (0.0f).xxx;
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    float4 v_14 = plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_12 = v_14.xyz;
    v_13 = v_14.w;
  } else {
    v_12 = mul(params.yuvToRgbConversionMatrix, float4(plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, plane_1.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f));
    v_13 = 1.0f;
  }
  float3 v_15 = v_12;
  float3 v_16 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_17 = params.srcTransferFunction;
    tint_TransferFunctionParams v_18 = params.dstTransferFunction;
    v_16 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_15, v_17), params.gamutConversionMatrix), v_18);
  } else {
    v_16 = v_15;
  }
  return float4(v_16, v_13);
}

float3x2 v_19(uint start_byte_offset) {
  uint4 v_20 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_21 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy)));
}

float3x3 v_23(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_24(uint start_byte_offset) {
  tint_TransferFunctionParams v_25 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_25;
}

float3x4 v_26(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_27(uint start_byte_offset) {
  uint v_28 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_29 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_30 = v_26((16u + start_byte_offset));
  tint_TransferFunctionParams v_31 = v_24((64u + start_byte_offset));
  tint_TransferFunctionParams v_32 = v_24((96u + start_byte_offset));
  float3x3 v_33 = v_23((128u + start_byte_offset));
  float3x2 v_34 = v_19((176u + start_byte_offset));
  float3x2 v_35 = v_19((200u + start_byte_offset));
  uint4 v_36 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_37 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_38 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_39 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_40 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_41 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_42 = {v_28, v_29, v_30, v_31, v_32, v_33, v_34, v_35, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_36.zw, v_36.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_37.zw, v_37.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_38.zw, v_38.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_39.zw, v_39.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_40.zw, v_40.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy))};
  return v_42;
}

float4 textureSampleBaseClampToEdge_7c04e6() {
  tint_ExternalTextureParams v_43 = v_27(0u);
  float4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_43, arg_1, (1.0f).xx);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(textureSampleBaseClampToEdge_7c04e6()));
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
cbuffer cbuffer_arg_0_params : register(b3, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t2, space1);
SamplerState arg_1 : register(s1, space1);
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

float4 tint_TextureSampleClampToEdgeMultiplanarExternal(Texture2D<float4> plane_0, Texture2D<float4> plane_1, tint_ExternalTextureParams params, SamplerState tint_sampler, float2 coords) {
  float2 v_11 = mul(float3(coords, 1.0f), params.sampleTransform);
  float3 v_12 = (0.0f).xxx;
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    float4 v_14 = plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_12 = v_14.xyz;
    v_13 = v_14.w;
  } else {
    v_12 = mul(params.yuvToRgbConversionMatrix, float4(plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, plane_1.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f));
    v_13 = 1.0f;
  }
  float3 v_15 = v_12;
  float3 v_16 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_17 = params.srcTransferFunction;
    tint_TransferFunctionParams v_18 = params.dstTransferFunction;
    v_16 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_15, v_17), params.gamutConversionMatrix), v_18);
  } else {
    v_16 = v_15;
  }
  return float4(v_16, v_13);
}

float3x2 v_19(uint start_byte_offset) {
  uint4 v_20 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_21 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy)));
}

float3x3 v_23(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_24(uint start_byte_offset) {
  tint_TransferFunctionParams v_25 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_25;
}

float3x4 v_26(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_27(uint start_byte_offset) {
  uint v_28 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_29 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_30 = v_26((16u + start_byte_offset));
  tint_TransferFunctionParams v_31 = v_24((64u + start_byte_offset));
  tint_TransferFunctionParams v_32 = v_24((96u + start_byte_offset));
  float3x3 v_33 = v_23((128u + start_byte_offset));
  float3x2 v_34 = v_19((176u + start_byte_offset));
  float3x2 v_35 = v_19((200u + start_byte_offset));
  uint4 v_36 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_37 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_38 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_39 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_40 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_41 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_42 = {v_28, v_29, v_30, v_31, v_32, v_33, v_34, v_35, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_36.zw, v_36.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_37.zw, v_37.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_38.zw, v_38.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_39.zw, v_39.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_40.zw, v_40.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy))};
  return v_42;
}

float4 textureSampleBaseClampToEdge_7c04e6() {
  tint_ExternalTextureParams v_43 = v_27(0u);
  float4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_43, arg_1, (1.0f).xx);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(textureSampleBaseClampToEdge_7c04e6()));
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


cbuffer cbuffer_arg_0_params : register(b3, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t2, space1);
SamplerState arg_1 : register(s1, space1);
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

float4 tint_TextureSampleClampToEdgeMultiplanarExternal(Texture2D<float4> plane_0, Texture2D<float4> plane_1, tint_ExternalTextureParams params, SamplerState tint_sampler, float2 coords) {
  float2 v_11 = mul(float3(coords, 1.0f), params.sampleTransform);
  float3 v_12 = (0.0f).xxx;
  float v_13 = 0.0f;
  if ((params.numPlanes == 1u)) {
    float4 v_14 = plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f);
    v_12 = v_14.xyz;
    v_13 = v_14.w;
  } else {
    v_12 = mul(params.yuvToRgbConversionMatrix, float4(plane_0.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane0RectMin, params.samplePlane0RectMax), 0.0f).x, plane_1.SampleLevel(tint_sampler, clamp(v_11, params.samplePlane1RectMin, params.samplePlane1RectMax), 0.0f).xy, 1.0f));
    v_13 = 1.0f;
  }
  float3 v_15 = v_12;
  float3 v_16 = (0.0f).xxx;
  if ((params.doYuvToRgbConversionOnly == 0u)) {
    tint_TransferFunctionParams v_17 = params.srcTransferFunction;
    tint_TransferFunctionParams v_18 = params.dstTransferFunction;
    v_16 = tint_ApplyGammaTransferFunction(mul(tint_ApplySrcTransferFunction(v_15, v_17), params.gamutConversionMatrix), v_18);
  } else {
    v_16 = v_15;
  }
  return float4(v_16, v_13);
}

float3x2 v_19(uint start_byte_offset) {
  uint4 v_20 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_21 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy)));
}

float3x3 v_23(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_24(uint start_byte_offset) {
  tint_TransferFunctionParams v_25 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_25;
}

float3x4 v_26(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_27(uint start_byte_offset) {
  uint v_28 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_29 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_30 = v_26((16u + start_byte_offset));
  tint_TransferFunctionParams v_31 = v_24((64u + start_byte_offset));
  tint_TransferFunctionParams v_32 = v_24((96u + start_byte_offset));
  float3x3 v_33 = v_23((128u + start_byte_offset));
  float3x2 v_34 = v_19((176u + start_byte_offset));
  float3x2 v_35 = v_19((200u + start_byte_offset));
  uint4 v_36 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_37 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_38 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_39 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_40 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_41 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_42 = {v_28, v_29, v_30, v_31, v_32, v_33, v_34, v_35, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_36.zw, v_36.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_37.zw, v_37.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_38.zw, v_38.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_39.zw, v_39.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_40.zw, v_40.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy))};
  return v_42;
}

float4 textureSampleBaseClampToEdge_7c04e6() {
  tint_ExternalTextureParams v_43 = v_27(0u);
  float4 res = tint_TextureSampleClampToEdgeMultiplanarExternal(arg_0_plane0, arg_0_plane1, v_43, arg_1, (1.0f).xx);
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_44 = (VertexOutput)0;
  v_44.pos = (0.0f).xxxx;
  v_44.prevent_dce = textureSampleBaseClampToEdge_7c04e6();
  VertexOutput v_45 = v_44;
  return v_45;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_46 = vertex_main_inner();
  vertex_main_outputs v_47 = {v_46.prevent_dce, v_46.pos};
  return v_47;
}

