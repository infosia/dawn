//
// main1
//
#version 310 es
precision highp float;
precision highp int;

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
float main1_inner() {
  return 1.0f;
}
void main() {
  float v = main1_inner();
  gl_FragDepth = clamp(v, tint_frag_depth_min, tint_frag_depth_max);
}
//
// main2
//
#version 310 es
#extension GL_OES_sample_variables: require
precision highp float;
precision highp int;

uint main2_inner() {
  return 1u;
}
void main() {
  gl_SampleMask[0u] = int(main2_inner());
}
