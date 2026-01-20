//
// any
//
#version 310 es
precision highp float;
precision highp int;


struct FragDepthAnyOutput {
  float frag_depth;
};

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
FragDepthAnyOutput any_inner() {
  return FragDepthAnyOutput(1.0f);
}
void main() {
  float v = any_inner().frag_depth;
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
//
// less
//
#version 310 es
#extension GL_EXT_conservative_depth: require
precision highp float;
precision highp int;


struct FragDepthLessOutput {
  float frag_depth;
};

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
layout(depth_less) out float gl_FragDepth;
FragDepthLessOutput less_inner() {
  return FragDepthLessOutput(1.0f);
}
void main() {
  float v = less_inner().frag_depth;
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
//
// greater
//
#version 310 es
#extension GL_EXT_conservative_depth: require
precision highp float;
precision highp int;


struct FragDepthGreaterOutput {
  float frag_depth;
};

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
layout(depth_greater) out float gl_FragDepth;
FragDepthGreaterOutput greater_inner() {
  return FragDepthGreaterOutput(1.0f);
}
void main() {
  float v = greater_inner().frag_depth;
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
