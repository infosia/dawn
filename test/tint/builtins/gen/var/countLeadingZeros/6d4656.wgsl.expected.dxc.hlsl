//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int countLeadingZeros_6d4656() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select((v <= 65535u), 16u, 0u) | (select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u) | (select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u) | (select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u) | (select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) <= 2147483647u), 1u, 0u) | select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)))))) + select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(countLeadingZeros_6d4656()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int countLeadingZeros_6d4656() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select((v <= 65535u), 16u, 0u) | (select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u) | (select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u) | (select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u) | (select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) <= 2147483647u), 1u, 0u) | select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)))))) + select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(countLeadingZeros_6d4656()));
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  int prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation int VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


int countLeadingZeros_6d4656() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select((v <= 65535u), 16u, 0u) | (select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u) | (select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u) | (select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u) | (select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) <= 2147483647u), 1u, 0u) | select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)))))) + select((((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) << select(((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) << select((((v << select((v <= 65535u), 16u, 0u)) << select(((v << select((v <= 65535u), 16u, 0u)) <= 16777215u), 8u, 0u)) <= 268435455u), 4u, 0u)) <= 1073741823u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = countLeadingZeros_6d4656();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

