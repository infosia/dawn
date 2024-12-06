#version 310 es


struct DeclaredAfterUsage {
  float f;
};

layout(binding = 0, std140)
uniform declared_after_usage_block_1_ubo {
  DeclaredAfterUsage inner;
} v_v;
vec4 tint_symbol_inner() {
  return vec4(v_v.inner.f);
}
void main() {
  gl_Position = tint_symbol_inner();
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
