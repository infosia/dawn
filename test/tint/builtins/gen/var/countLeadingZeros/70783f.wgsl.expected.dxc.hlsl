//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 countLeadingZeros_70783f() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = ((select((v <= (65535u).xx), (16u).xx, (0u).xx) | (select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx) | (select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx) | (select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx) | (select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) <= (2147483647u).xx), (1u).xx, (0u).xx) | select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx)))))) + select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx));
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, countLeadingZeros_70783f());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 countLeadingZeros_70783f() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = ((select((v <= (65535u).xx), (16u).xx, (0u).xx) | (select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx) | (select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx) | (select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx) | (select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) <= (2147483647u).xx), (1u).xx, (0u).xx) | select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx)))))) + select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, countLeadingZeros_70783f());
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  uint2 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation uint2 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


uint2 countLeadingZeros_70783f() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = ((select((v <= (65535u).xx), (16u).xx, (0u).xx) | (select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx) | (select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx) | (select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx) | (select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) <= (2147483647u).xx), (1u).xx, (0u).xx) | select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx)))))) + select((((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) << select(((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) << select((((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) << select(((v << select((v <= (65535u).xx), (16u).xx, (0u).xx)) <= (16777215u).xx), (8u).xx, (0u).xx)) <= (268435455u).xx), (4u).xx, (0u).xx)) <= (1073741823u).xx), (2u).xx, (0u).xx)) == (0u).xx), (1u).xx, (0u).xx));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = countLeadingZeros_70783f();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

