#version 310 es
precision highp float;
precision highp int;


struct tint_push_constant_struct {
  uint tint_frag_depth_min;
  uint tint_frag_depth_max;
};

struct tint_symbol_1 {
  tint_push_constant_struct tint_symbol;
};

layout(location = 0) uniform tint_symbol_1 v;
float main1_inner() {
  return 1.0f;
}
void main() {
  gl_FragDepth = main1_inner();
}
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
