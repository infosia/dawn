#version 310 es

layout(binding = 0, std140)
uniform v_declared_after_usage_block_ubo {
  uvec4 inner[1];
} v;
vec4 main_inner() {
  return vec4(uintBitsToFloat(v.inner[0u].x));
}
void main() {
  vec4 v_1 = main_inner();
  gl_Position = vec4(v_1.x, -(v_1.y), ((2.0f * v_1.z) - v_1.w), v_1.w);
  gl_PointSize = 1.0f;
}
