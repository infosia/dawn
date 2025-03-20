#version 310 es
precision highp float;
precision highp int;


struct main_out {
  vec4 out_var_1;
};

vec4 in_var = vec4(0.0f);
vec4 out_var = vec4(0.0f);
layout(location = 0) out vec4 main_loc0_Output;
void main_1() {
}
main_out main_inner(vec4 in_var_param) {
  in_var = in_var_param;
  main_1();
  return main_out(out_var);
}
void main() {
  main_loc0_Output = main_inner(gl_FragCoord).out_var_1;
}
