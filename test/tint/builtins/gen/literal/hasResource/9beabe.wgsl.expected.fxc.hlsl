//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture3D<int4> tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_9beabe() {
  bool v = false;
  if ((1u < tint_resource_table_metadata.Load(0u))) {
    v = (tint_resource_table_metadata.Load(8u) == 11u);
  } else {
    v = false;
  }
  bool res = v;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(hasResource_9beabe()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture3D<int4> tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_9beabe() {
  bool v = false;
  if ((1u < tint_resource_table_metadata.Load(0u))) {
    v = (tint_resource_table_metadata.Load(8u) == 11u);
  } else {
    v = false;
  }
  bool res = v;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(hasResource_9beabe()));
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


Texture3D<int4> tint_resource_table_array[] : register(t41, space42);
ByteAddressBuffer tint_resource_table_metadata : register(t42, space42);
int hasResource_9beabe() {
  bool v = false;
  if ((1u < tint_resource_table_metadata.Load(0u))) {
    v = (tint_resource_table_metadata.Load(8u) == 11u);
  } else {
    v = false;
  }
  bool res = v;
  return ((all((res == false))) ? (int(1)) : (int(0)));
}

VertexOutput vertex_main_inner() {
  VertexOutput v_1 = (VertexOutput)0;
  v_1.pos = (0.0f).xxxx;
  v_1.prevent_dce = hasResource_9beabe();
  VertexOutput v_2 = v_1;
  return v_2;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_3 = vertex_main_inner();
  vertex_main_outputs v_4 = {v_3.prevent_dce, v_3.pos};
  return v_4;
}

