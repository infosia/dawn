#version 310 es


struct tint_push_constant_struct {
  uint tint_first_instance;
};

struct tint_symbol_2 {
  tint_push_constant_struct tint_symbol_1;
};

layout(location = 0) uniform tint_symbol_2 v;
vec4 tint_symbol_inner(uint vertex_index, uint instance_index) {
  uint foo = (vertex_index + instance_index);
  return vec4(0.0f);
}
void main() {
  uint v_1 = uint(gl_VertexID);
  gl_Position = tint_symbol_inner(v_1, uint(gl_InstanceID));
  gl_Position[1u] = -(gl_Position.y);
  gl_Position[2u] = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
