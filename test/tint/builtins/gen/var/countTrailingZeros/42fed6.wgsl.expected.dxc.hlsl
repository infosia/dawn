//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int countTrailingZeros_42fed6() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select(((v & 65535u) == 0u), 16u, 0u) | (select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u) | (select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u) | (select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u) | select(((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) & 1u) == 0u), 1u, 0u))))) + select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(countTrailingZeros_42fed6()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
int countTrailingZeros_42fed6() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select(((v & 65535u) == 0u), 16u, 0u) | (select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u) | (select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u) | (select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u) | select(((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) & 1u) == 0u), 1u, 0u))))) + select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(countTrailingZeros_42fed6()));
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


int countTrailingZeros_42fed6() {
  int arg_0 = int(1);
  uint v = asuint(arg_0);
  int res = asint(((select(((v & 65535u) == 0u), 16u, 0u) | (select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u) | (select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u) | (select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u) | select(((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) & 1u) == 0u), 1u, 0u))))) + select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) >> select((((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) >> select(((((v >> select(((v & 65535u) == 0u), 16u, 0u)) >> select((((v >> select(((v & 65535u) == 0u), 16u, 0u)) & 255u) == 0u), 8u, 0u)) & 15u) == 0u), 4u, 0u)) & 3u) == 0u), 2u, 0u)) == 0u), 1u, 0u)));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = countTrailingZeros_42fed6();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

