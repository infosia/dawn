//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int firstLeadingBit_57a1a3() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u) | (select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u)))))));
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(firstLeadingBit_57a1a3()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int firstLeadingBit_57a1a3() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u) | (select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u)))))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(firstLeadingBit_57a1a3()));
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


int firstLeadingBit_57a1a3() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) == 0u), 4294967295u, (select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u) | (select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u) | (select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u) | (select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u) | select(((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) >> select((((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) >> select(((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) >> select((((select((v < 2147483648u), v, ~(v)) >> select(((select((v < 2147483648u), v, ~(v)) & 4294901760u) == 0u), 0u, 16u)) & 65280u) == 0u), 0u, 8u)) & 240u) == 0u), 0u, 4u)) & 12u) == 0u), 0u, 2u)) & 2u) == 0u), 0u, 1u)))))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = firstLeadingBit_57a1a3();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

