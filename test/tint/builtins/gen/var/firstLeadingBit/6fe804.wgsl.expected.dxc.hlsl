//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 firstLeadingBit_6fe804() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) == (0u).xx), (4294967295u).xx, (select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx) | (select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx) | (select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx) | (select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx) | select(((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) & (2u).xx) == (0u).xx), (0u).xx, (1u).xx))))));
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, firstLeadingBit_6fe804());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint2 firstLeadingBit_6fe804() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) == (0u).xx), (4294967295u).xx, (select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx) | (select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx) | (select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx) | (select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx) | select(((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) & (2u).xx) == (0u).xx), (0u).xx, (1u).xx))))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, firstLeadingBit_6fe804());
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


uint2 firstLeadingBit_6fe804() {
  uint2 arg_0 = (1u).xx;
  uint2 v = arg_0;
  uint2 res = select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) == (0u).xx), (4294967295u).xx, (select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx) | (select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx) | (select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx) | (select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx) | select(((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) >> select((((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) >> select(((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) >> select((((v >> select(((v & (4294901760u).xx) == (0u).xx), (0u).xx, (16u).xx)) & (65280u).xx) == (0u).xx), (0u).xx, (8u).xx)) & (240u).xx) == (0u).xx), (0u).xx, (4u).xx)) & (12u).xx) == (0u).xx), (0u).xx, (2u).xx)) & (2u).xx) == (0u).xx), (0u).xx, (1u).xx))))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = firstLeadingBit_6fe804();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

