//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture2D tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_c93d9f() {
  uint v = uint(int(1));
  bool v_1 = false;
  if ((v < tint_resource_table_metadata.Load(0u))) {
    v_1 = (tint_resource_table_metadata.Load((4u + (v * 4u))) == 22u);
  } else {
    v_1 = false;
  }
  bool res = v_1;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(hasResource_c93d9f()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture2D tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_c93d9f() {
  uint v = uint(int(1));
  bool v_1 = false;
  if ((v < tint_resource_table_metadata.Load(0u))) {
    v_1 = (tint_resource_table_metadata.Load((4u + (v * 4u))) == 22u);
  } else {
    v_1 = false;
  }
  bool res = v_1;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(hasResource_c93d9f()));
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


Texture2D tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_c93d9f() {
  uint v = uint(int(1));
  bool v_1 = false;
  if ((v < tint_resource_table_metadata.Load(0u))) {
    v_1 = (tint_resource_table_metadata.Load((4u + (v * 4u))) == 22u);
  } else {
    v_1 = false;
  }
  bool res = v_1;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

VertexOutput vertex_main_inner() {
  VertexOutput v_2 = (VertexOutput)0;
  v_2.pos = (0.0f).xxxx;
  v_2.prevent_dce = hasResource_c93d9f();
  VertexOutput v_3 = v_2;
  return v_3;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_4 = vertex_main_inner();
  vertex_main_outputs v_5 = {v_4.prevent_dce, v_4.pos};
  return v_5;
}

