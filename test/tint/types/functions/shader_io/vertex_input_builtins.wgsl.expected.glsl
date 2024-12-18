#version 310 es

vec4 main_inner(uint vertex_index, uint instance_index) {
  uint foo = (vertex_index + instance_index);
  return vec4(0.0f);
}
void main() {
  uint v = uint(gl_VertexID);
  gl_Position = main_inner(v, uint(gl_InstanceID));
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
