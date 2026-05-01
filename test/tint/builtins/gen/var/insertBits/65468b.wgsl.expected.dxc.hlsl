//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int insertBits_65468b() {
  int arg_0 = int(1);
  int arg_1 = int(1);
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int v = arg_0;
  int v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint(v_2))), int(0));
  int v_5 = (v_4 & int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))));
  int res = (v_5 | (v & int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))));
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(insertBits_65468b()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int insertBits_65468b() {
  int arg_0 = int(1);
  int arg_1 = int(1);
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int v = arg_0;
  int v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint(v_2))), int(0));
  int v_5 = (v_4 & int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))));
  int res = (v_5 | (v & int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(insertBits_65468b()));
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


int insertBits_65468b() {
  int arg_0 = int(1);
  int arg_1 = int(1);
  uint arg_2 = 1u;
  uint arg_3 = 1u;
  int v = arg_0;
  int v_1 = arg_1;
  uint v_2 = arg_2;
  uint v_3 = (min(v_2, 32u) + min(arg_3, 32u));
  int v_4 = select((v_2 < 32u), asint((asuint(v_1) << uint(v_2))), int(0));
  int v_5 = (v_4 & int(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))));
  int res = (v_5 | (v & int(~(((select((v_2 < 32u), (1u << v_2), 0u) - 1u) ^ (select((v_3 < 32u), (1u << v_3), 0u) - 1u))))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_6 = (VertexOutput)0;
  v_6.pos = (0.0f).xxxx;
  v_6.prevent_dce = insertBits_65468b();
  VertexOutput v_7 = v_6;
  return v_7;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_8 = vertex_main_inner();
  vertex_main_outputs v_9 = {v_8.prevent_dce, v_8.pos};
  return v_9;
}

