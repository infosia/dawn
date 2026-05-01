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
float3x2 v(uint start_byte_offset) {
  uint4 v_1 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_2 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

float3x3 v_4(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_5(uint start_byte_offset) {
  tint_TransferFunctionParams v_6 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_6;
}

float3x4 v_7(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_8(uint start_byte_offset) {
  uint v_9 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_10 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_11 = v_7((16u + start_byte_offset));
  tint_TransferFunctionParams v_12 = v_5((64u + start_byte_offset));
  tint_TransferFunctionParams v_13 = v_5((96u + start_byte_offset));
  float3x3 v_14 = v_4((128u + start_byte_offset));
  float3x2 v_15 = v((176u + start_byte_offset));
  float3x2 v_16 = v((200u + start_byte_offset));
  uint4 v_17 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_18 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_19 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_20 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_21 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_23 = {v_9, v_10, v_11, v_12, v_13, v_14, v_15, v_16, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_17.zw, v_17.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_18.zw, v_18.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_19.zw, v_19.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy))};
  return v_23;
}

uint2 textureDimensions_cdc6c9() {
  tint_ExternalTextureParams v_24 = v_8(0u);
  uint2 res = (v_24.apparentSize + (1u).xx);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, textureDimensions_cdc6c9());
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
float3x2 v(uint start_byte_offset) {
  uint4 v_1 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_2 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

float3x3 v_4(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_5(uint start_byte_offset) {
  tint_TransferFunctionParams v_6 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_6;
}

float3x4 v_7(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_8(uint start_byte_offset) {
  uint v_9 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_10 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_11 = v_7((16u + start_byte_offset));
  tint_TransferFunctionParams v_12 = v_5((64u + start_byte_offset));
  tint_TransferFunctionParams v_13 = v_5((96u + start_byte_offset));
  float3x3 v_14 = v_4((128u + start_byte_offset));
  float3x2 v_15 = v((176u + start_byte_offset));
  float3x2 v_16 = v((200u + start_byte_offset));
  uint4 v_17 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_18 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_19 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_20 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_21 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_23 = {v_9, v_10, v_11, v_12, v_13, v_14, v_15, v_16, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_17.zw, v_17.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_18.zw, v_18.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_19.zw, v_19.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy))};
  return v_23;
}

uint2 textureDimensions_cdc6c9() {
  tint_ExternalTextureParams v_24 = v_8(0u);
  uint2 res = (v_24.apparentSize + (1u).xx);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, textureDimensions_cdc6c9());
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
  uint2 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation uint2 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


cbuffer cbuffer_arg_0_params : register(b2, space1) {
  uint4 arg_0_params[17];
};
Texture2D<float4> arg_0_plane0 : register(t0, space1);
Texture2D<float4> arg_0_plane1 : register(t1, space1);
float3x2 v(uint start_byte_offset) {
  uint4 v_1 = arg_0_params[(start_byte_offset / 16u)];
  uint4 v_2 = arg_0_params[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = arg_0_params[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

float3x3 v_4(uint start_byte_offset) {
  return float3x3(asfloat(arg_0_params[(start_byte_offset / 16u)].xyz), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)].xyz), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)].xyz));
}

tint_TransferFunctionParams v_5(uint start_byte_offset) {
  tint_TransferFunctionParams v_6 = {arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)], asfloat(arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((20u + start_byte_offset) / 16u)][(((20u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((24u + start_byte_offset) / 16u)][(((24u + start_byte_offset) & 15u) >> 2u)]), asfloat(arg_0_params[((28u + start_byte_offset) / 16u)][(((28u + start_byte_offset) & 15u) >> 2u)])};
  return v_6;
}

float3x4 v_7(uint start_byte_offset) {
  return float3x4(asfloat(arg_0_params[(start_byte_offset / 16u)]), asfloat(arg_0_params[((16u + start_byte_offset) / 16u)]), asfloat(arg_0_params[((32u + start_byte_offset) / 16u)]));
}

tint_ExternalTextureParams v_8(uint start_byte_offset) {
  uint v_9 = arg_0_params[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)];
  uint v_10 = arg_0_params[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)];
  float3x4 v_11 = v_7((16u + start_byte_offset));
  tint_TransferFunctionParams v_12 = v_5((64u + start_byte_offset));
  tint_TransferFunctionParams v_13 = v_5((96u + start_byte_offset));
  float3x3 v_14 = v_4((128u + start_byte_offset));
  float3x2 v_15 = v((176u + start_byte_offset));
  float3x2 v_16 = v((200u + start_byte_offset));
  uint4 v_17 = arg_0_params[((224u + start_byte_offset) / 16u)];
  uint4 v_18 = arg_0_params[((232u + start_byte_offset) / 16u)];
  uint4 v_19 = arg_0_params[((240u + start_byte_offset) / 16u)];
  uint4 v_20 = arg_0_params[((248u + start_byte_offset) / 16u)];
  uint4 v_21 = arg_0_params[((256u + start_byte_offset) / 16u)];
  uint4 v_22 = arg_0_params[((264u + start_byte_offset) / 16u)];
  tint_ExternalTextureParams v_23 = {v_9, v_10, v_11, v_12, v_13, v_14, v_15, v_16, asfloat(select(((((224u + start_byte_offset) & 15u) >> 2u) == 2u), v_17.zw, v_17.xy)), asfloat(select(((((232u + start_byte_offset) & 15u) >> 2u) == 2u), v_18.zw, v_18.xy)), asfloat(select(((((240u + start_byte_offset) & 15u) >> 2u) == 2u), v_19.zw, v_19.xy)), asfloat(select(((((248u + start_byte_offset) & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), select(((((256u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy), asfloat(select(((((264u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy))};
  return v_23;
}

uint2 textureDimensions_cdc6c9() {
  tint_ExternalTextureParams v_24 = v_8(0u);
  uint2 res = (v_24.apparentSize + (1u).xx);
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_25 = (VertexOutput)0;
  v_25.pos = (0.0f).xxxx;
  v_25.prevent_dce = textureDimensions_cdc6c9();
  VertexOutput v_26 = v_25;
  return v_26;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_27 = vertex_main_inner();
  vertex_main_outputs v_28 = {v_27.prevent_dce, v_27.pos};
  return v_28;
}

