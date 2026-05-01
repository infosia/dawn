//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint firstLeadingBit_f0779d() {
  uint arg_0 = 1u;
  uint v = arg_0;
  uint res = select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((v & 4294901760u) == 0u), 0u, 16u) | (select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u))))));
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, firstLeadingBit_f0779d());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint firstLeadingBit_f0779d() {
  uint arg_0 = 1u;
  uint v = arg_0;
  uint res = select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((v & 4294901760u) == 0u), 0u, 16u) | (select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u))))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, firstLeadingBit_f0779d());
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  uint prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation uint VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


uint firstLeadingBit_f0779d() {
  uint arg_0 = 1u;
  uint v = arg_0;
  uint res = select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((v & 4294901760u) == 0u), 0u, 16u) | (select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) >> select((((v >> select(((v & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u))))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = firstLeadingBit_f0779d();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

