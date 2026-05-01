//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint3 firstLeadingBit_3fd7d0() {
  uint3 arg_0 = (1u).xxx;
  uint3 v = arg_0;
  uint3 res = select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) == (0u).xxx), (4294967295u).xxx, (select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx) | (select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx) | (select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx) | (select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx) | select(((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) & (2u).xxx) == (0u).xxx), (0u).xxx, (1u).xxx))))));
  return res;
}

void fragment_main() {
  prevent_dce.Store3(0u, firstLeadingBit_3fd7d0());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
uint3 firstLeadingBit_3fd7d0() {
  uint3 arg_0 = (1u).xxx;
  uint3 v = arg_0;
  uint3 res = select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) == (0u).xxx), (4294967295u).xxx, (select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx) | (select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx) | (select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx) | (select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx) | select(((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) & (2u).xxx) == (0u).xxx), (0u).xxx, (1u).xxx))))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store3(0u, firstLeadingBit_3fd7d0());
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  uint3 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation uint3 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


uint3 firstLeadingBit_3fd7d0() {
  uint3 arg_0 = (1u).xxx;
  uint3 v = arg_0;
  uint3 res = select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) == (0u).xxx), (4294967295u).xxx, (select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx) | (select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx) | (select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx) | (select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx) | select(((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) >> select((((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) >> select(((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) >> select((((v >> select(((v & (4294901760u).xxx) == (0u).xxx), (0u).xxx, (16u).xxx)) & (65280u).xxx) == (0u).xxx), (0u).xxx, (8u).xxx)) & (240u).xxx) == (0u).xxx), (0u).xxx, (4u).xxx)) & (12u).xxx) == (0u).xxx), (0u).xxx, (2u).xxx)) & (2u).xxx) == (0u).xxx), (0u).xxx, (1u).xxx))))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = firstLeadingBit_3fd7d0();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

