//
// any
//
#version 310 es
precision highp float;
precision highp int;

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
float any_inner() {
  return 1.0f;
}
void main() {
  float v = any_inner();
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
//
// less
//
#version 310 es
#extension GL_EXT_conservative_depth: require
precision highp float;
precision highp int;

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
layout(depth_less) out float gl_FragDepth;
float less_inner() {
  return 1.0f;
}
void main() {
  float v = less_inner();
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
//
// greater
//
#version 310 es
#extension GL_EXT_conservative_depth: require
precision highp float;
precision highp int;

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
layout(depth_greater) out float gl_FragDepth;
float greater_inner() {
  return 1.0f;
}
void main() {
  float v = greater_inner();
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
