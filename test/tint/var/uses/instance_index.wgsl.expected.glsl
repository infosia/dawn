#version 310 es

vec4 main_inner(uint b) {
  return vec4(float(b));
}
void main() {
  gl_Position = main_inner(uint(gl_InstanceID));
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
