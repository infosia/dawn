#version 310 es


struct tint_immediate_struct {
  float inner;
};

layout(location = 4) uniform tint_immediate_struct tint_immediates;
layout(location = 0) uniform uint tint_first_instance;
vec4 main_inner(uint b) {
  float v = tint_immediates.inner;
  return vec4((v + float(b)));
}
void main() {
  uint v_1 = uint(gl_InstanceID);
  vec4 v_2 = main_inner((v_1 + tint_first_instance));
  gl_Position = vec4(v_2.x, -(v_2.y), ((2.0f * v_2.z) - v_2.w), v_2.w);
  gl_PointSize = 1.0f;
}
