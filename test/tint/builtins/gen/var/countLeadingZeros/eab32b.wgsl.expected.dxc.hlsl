//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int4 countLeadingZeros_eab32b() {
  int4 arg_0 = (int(1)).xxxx;
  uint4 v = asuint(arg_0);
  int4 res = asint(((select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx) | (select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx) | (select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx) | (select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx) | (select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) <= (2147483647u).xxxx), (1u).xxxx, (0u).xxxx) | select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)))))) + select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)));
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(countLeadingZeros_eab32b()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int4 countLeadingZeros_eab32b() {
  int4 arg_0 = (int(1)).xxxx;
  uint4 v = asuint(arg_0);
  int4 res = asint(((select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx) | (select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx) | (select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx) | (select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx) | (select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) <= (2147483647u).xxxx), (1u).xxxx, (0u).xxxx) | select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)))))) + select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(countLeadingZeros_eab32b()));
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  int4 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation int4 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


int4 countLeadingZeros_eab32b() {
  int4 arg_0 = (int(1)).xxxx;
  uint4 v = asuint(arg_0);
  int4 res = asint(((select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx) | (select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx) | (select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx) | (select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx) | (select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) <= (2147483647u).xxxx), (1u).xxxx, (0u).xxxx) | select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)))))) + select((((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) << select(((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) << select((((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) << select(((v << select((v <= (65535u).xxxx), (16u).xxxx, (0u).xxxx)) <= (16777215u).xxxx), (8u).xxxx, (0u).xxxx)) <= (268435455u).xxxx), (4u).xxxx, (0u).xxxx)) <= (1073741823u).xxxx), (2u).xxxx, (0u).xxxx)) == (0u).xxxx), (1u).xxxx, (0u).xxxx)));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = countLeadingZeros_eab32b();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

