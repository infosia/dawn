//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 insertBits_3c7ba5() {
  uint2 arg_0 = (1u).xx;
  uint2 arg_1 = (1u).xx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  uint2 v = arg_0;
  uint2 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  uint2 v_4 = select((v_2 < 32u), (v_1 << uint2((v_2).xx)), (0u).xx);
  uint2 v_5 = (v_4 & uint2((((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))).xx));
  uint2 res = (v_5 | (v & uint2((~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xx)));
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, insertBits_3c7ba5());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 insertBits_3c7ba5() {
  uint2 arg_0 = (1u).xx;
  uint2 arg_1 = (1u).xx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  uint2 v = arg_0;
  uint2 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  uint2 v_4 = select((v_2 < 32u), (v_1 << uint2((v_2).xx)), (0u).xx);
  uint2 v_5 = (v_4 & uint2((((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))).xx));
  uint2 res = (v_5 | (v & uint2((~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xx)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, insertBits_3c7ba5());
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


uint2 insertBits_3c7ba5() {
  uint2 arg_0 = (1u).xx;
  uint2 arg_1 = (1u).xx;
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  uint2 v = arg_0;
  uint2 v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  uint2 v_4 = select((v_2 < 32u), (v_1 << uint2((v_2).xx)), (0u).xx);
  uint2 v_5 = (v_4 & uint2((((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))).xx));
  uint2 res = (v_5 | (v & uint2((~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u)))).xx)));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_6 = (VertexOutput)0;
  v_6.pos = (0.0f).xxxx;
  v_6.prevent_dce = insertBits_3c7ba5();
  VertexOutput v_7 = v_6;
  return v_7;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_8 = vertex_main_inner();
  vertex_main_outputs v_9 = {v_8.prevent_dce, v_8.pos};
  return v_9;
}

