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

struct vertex_main_outputs {
  float4 tint_symbol : SV_Position;
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
  return (((abs(v) < v_2)) ? ((v_3 * ((params.C * abs(v)) + params.F))) : ((v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E))));
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

float4 textureLoad2d(Texture2D<float4> texture_plane0, Texture2D<float4> texture_plane1, tint_ExternalTextureParams texture_params, int2 coords) {
  return tint_TextureLoadMultiplanarExternal(texture_plane0, texture_plane1, texture_params, uint2(coords));
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  float2 v_26 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_25.zw) : (v_25.xy)));
  uint4 v_27 = arg_0_params[((8u + start_byte_offset) / 16u)];
  float2 v_28 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_27.zw) : (v_27.xy)));
  uint4 v_29 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(v_26, v_28, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_29.zw) : (v_29.xy))));
}

float3x3 v_30(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_31(uint start_byte_offset) {
  tint_TransferFunctionParams v_32 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_32;
}

float3x4 v_33(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_34(uint start_byte_offset) {
  uint v_35 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_36 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_37 = v_33((16u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_31((64u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_31((96u + start_byte_offset));
  float3x3 v_40 = v_30((128u + start_byte_offset));
  float3x2 v_41 = v_24((176u + start_byte_offset));
  float3x2 v_42 = v_24((200u + start_byte_offset));
  uint4 v_43 = arg_0_params[((224u + start_byte_offset) / 16u)];
  float2 v_44 = asfloat(((((((224u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_43.zw) : (v_43.xy)));
  uint4 v_45 = arg_0_params[((232u + start_byte_offset) / 16u)];
  float2 v_46 = asfloat(((((((232u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_45.zw) : (v_45.xy)));
  uint4 v_47 = arg_0_params[((240u + start_byte_offset) / 16u)];
  float2 v_48 = asfloat(((((((240u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_47.zw) : (v_47.xy)));
  uint4 v_49 = arg_0_params[((248u + start_byte_offset) / 16u)];
  float2 v_50 = asfloat(((((((248u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_49.zw) : (v_49.xy)));
  uint4 v_51 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint2 v_52 = ((((((256u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_51.zw) : (v_51.xy));
  uint4 v_53 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_54 = {v_35, v_36, v_37, v_38, v_39, v_40, v_41, v_42, v_44, v_46, v_48, v_50, v_52, asfloat(((((((264u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_53.zw) : (v_53.xy)))};
  return v_54;
}

void doTextureLoad() {
  tint_ExternalTextureParams v_55 = v_34(0u);
  float4 res = textureLoad2d(arg_0_plane0, arg_0_plane1, v_55, (int(0)).xx);
}

float4 vertex_main_inner() {
  doTextureLoad();
  return (0.0f).xxxx;
}

vertex_main_outputs vertex_main() {
  vertex_main_outputs v_56 = {vertex_main_inner()};
  return v_56;
}

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
  return (((abs(v) < v_2)) ? ((v_3 * ((params.C * abs(v)) + params.F))) : ((v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E))));
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

float4 textureLoad2d(Texture2D<float4> texture_plane0, Texture2D<float4> texture_plane1, tint_ExternalTextureParams texture_params, int2 coords) {
  return tint_TextureLoadMultiplanarExternal(texture_plane0, texture_plane1, texture_params, uint2(coords));
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  float2 v_26 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_25.zw) : (v_25.xy)));
  uint4 v_27 = arg_0_params[((8u + start_byte_offset) / 16u)];
  float2 v_28 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_27.zw) : (v_27.xy)));
  uint4 v_29 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(v_26, v_28, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_29.zw) : (v_29.xy))));
}

float3x3 v_30(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_31(uint start_byte_offset) {
  tint_TransferFunctionParams v_32 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_32;
}

float3x4 v_33(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_34(uint start_byte_offset) {
  uint v_35 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_36 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_37 = v_33((16u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_31((64u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_31((96u + start_byte_offset));
  float3x3 v_40 = v_30((128u + start_byte_offset));
  float3x2 v_41 = v_24((176u + start_byte_offset));
  float3x2 v_42 = v_24((200u + start_byte_offset));
  uint4 v_43 = arg_0_params[((224u + start_byte_offset) / 16u)];
  float2 v_44 = asfloat(((((((224u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_43.zw) : (v_43.xy)));
  uint4 v_45 = arg_0_params[((232u + start_byte_offset) / 16u)];
  float2 v_46 = asfloat(((((((232u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_45.zw) : (v_45.xy)));
  uint4 v_47 = arg_0_params[((240u + start_byte_offset) / 16u)];
  float2 v_48 = asfloat(((((((240u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_47.zw) : (v_47.xy)));
  uint4 v_49 = arg_0_params[((248u + start_byte_offset) / 16u)];
  float2 v_50 = asfloat(((((((248u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_49.zw) : (v_49.xy)));
  uint4 v_51 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint2 v_52 = ((((((256u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_51.zw) : (v_51.xy));
  uint4 v_53 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_54 = {v_35, v_36, v_37, v_38, v_39, v_40, v_41, v_42, v_44, v_46, v_48, v_50, v_52, asfloat(((((((264u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_53.zw) : (v_53.xy)))};
  return v_54;
}

void doTextureLoad() {
  tint_ExternalTextureParams v_55 = v_34(0u);
  float4 res = textureLoad2d(arg_0_plane0, arg_0_plane1, v_55, (int(0)).xx);
}

void fragment_main() {
  doTextureLoad();
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
  return (((abs(v) < v_2)) ? ((v_3 * ((params.C * abs(v)) + params.F))) : ((v_3 * (pow(((params.A * abs(v)) + params.B), v_1) + params.E))));
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

float4 textureLoad2d(Texture2D<float4> texture_plane0, Texture2D<float4> texture_plane1, tint_ExternalTextureParams texture_params, int2 coords) {
  return tint_TextureLoadMultiplanarExternal(texture_plane0, texture_plane1, texture_params, uint2(coords));
}

float3x2 v_24(uint start_byte_offset) {
  uint4 v_25 = arg_0_params[(start_byte_offset / 16u)];
  float2 v_26 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_25.zw) : (v_25.xy)));
  uint4 v_27 = arg_0_params[((8u + start_byte_offset) / 16u)];
  float2 v_28 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_27.zw) : (v_27.xy)));
  uint4 v_29 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(v_26, v_28, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_29.zw) : (v_29.xy))));
}

float3x3 v_30(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_31(uint start_byte_offset) {
  tint_TransferFunctionParams v_32 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_32;
}

float3x4 v_33(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_34(uint start_byte_offset) {
  uint v_35 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_36 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_37 = v_33((16u + start_byte_offset));
  tint_TransferFunctionParams v_38 = v_31((64u + start_byte_offset));
  tint_TransferFunctionParams v_39 = v_31((96u + start_byte_offset));
  float3x3 v_40 = v_30((128u + start_byte_offset));
  float3x2 v_41 = v_24((176u + start_byte_offset));
  float3x2 v_42 = v_24((200u + start_byte_offset));
  uint4 v_43 = arg_0_params[((224u + start_byte_offset) / 16u)];
  float2 v_44 = asfloat(((((((224u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_43.zw) : (v_43.xy)));
  uint4 v_45 = arg_0_params[((232u + start_byte_offset) / 16u)];
  float2 v_46 = asfloat(((((((232u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_45.zw) : (v_45.xy)));
  uint4 v_47 = arg_0_params[((240u + start_byte_offset) / 16u)];
  float2 v_48 = asfloat(((((((240u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_47.zw) : (v_47.xy)));
  uint4 v_49 = arg_0_params[((248u + start_byte_offset) / 16u)];
  float2 v_50 = asfloat(((((((248u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_49.zw) : (v_49.xy)));
  uint4 v_51 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint2 v_52 = ((((((256u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_51.zw) : (v_51.xy));
  uint4 v_53 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_54 = {v_35, v_36, v_37, v_38, v_39, v_40, v_41, v_42, v_44, v_46, v_48, v_50, v_52, asfloat(((((((264u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_53.zw) : (v_53.xy)))};
  return v_54;
}

void doTextureLoad() {
  tint_ExternalTextureParams v_55 = v_34(0u);
  float4 res = textureLoad2d(arg_0_plane0, arg_0_plane1, v_55, (int(0)).xx);
}

[numthreads(1, 1, 1)]
void compute_main() {
  doTextureLoad();
}

