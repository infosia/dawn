#version 310 es
#extension GL_OES_sample_variables: require
precision highp float;
precision highp int;


struct FragmentOutputs {
  float frag_depth;
  uint sample_mask;
};

layout(location = 0) uniform float tint_frag_depth_min;
layout(location = 1) uniform float tint_frag_depth_max;
FragmentOutputs main_inner() {
  return FragmentOutputs(1.0f, 1u);
}
void main() {
  FragmentOutputs v = main_inner();
  gl_FragDepth = clamp(v.frag_depth, tint_frag_depth_min, tint_frag_depth_max);
  gl_SampleMask[0u] = int(v.sample_mask);
}
