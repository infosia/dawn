#version 310 es
#extension GL_OES_sample_variables: require
precision highp float;
precision highp int;


struct tint_push_constant_struct {
  uint tint_frag_depth_min;
  uint tint_frag_depth_max;
};

struct tint_symbol_2 {
  tint_push_constant_struct tint_symbol_1;
};

struct FragmentOutputs {
  float frag_depth;
  uint sample_mask;
};

layout(location = 0) uniform tint_symbol_2 v;
FragmentOutputs tint_symbol_inner() {
  return FragmentOutputs(1.0f, 1u);
}
void main() {
  FragmentOutputs v_1 = tint_symbol_inner();
  gl_FragDepth = v_1.frag_depth;
  gl_SampleMask[0u] = int(v_1.sample_mask);
}
