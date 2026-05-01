//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int4 insertBits_d86978() {
  int4 arg_0 = (int(1)).xxxx;
  int4 arg_1 = (int(1)).xxxx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int4 v = arg_0;
  int4 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int4 v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint4((v_2).xxxx))), (int(0)).xxxx);
  int4 v_5 = (v_4 & int4((int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xxxx));
  int4 res = (v_5 | (v & int4((int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))).xxxx)));
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(insertBits_d86978()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int4 insertBits_d86978() {
  int4 arg_0 = (int(1)).xxxx;
  int4 arg_1 = (int(1)).xxxx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int4 v = arg_0;
  int4 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int4 v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint4((v_2).xxxx))), (int(0)).xxxx);
  int4 v_5 = (v_4 & int4((int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xxxx));
  int4 res = (v_5 | (v & int4((int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))).xxxx)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(insertBits_d86978()));
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


int4 insertBits_d86978() {
  int4 arg_0 = (int(1)).xxxx;
  int4 arg_1 = (int(1)).xxxx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int4 v = arg_0;
  int4 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int4 v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint4((v_2).xxxx))), (int(0)).xxxx);
  int4 v_5 = (v_4 & int4((int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xxxx));
  int4 res = (v_5 | (v & int4((int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))).xxxx)));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_6 = (VertexOutput)0;
  v_6.pos = (0.0f).xxxx;
  v_6.prevent_dce = insertBits_d86978();
  VertexOutput v_7 = v_6;
  return v_7;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_8 = vertex_main_inner();
  vertex_main_outputs v_9 = {v_8.prevent_dce, v_8.pos};
  return v_9;
}

