#version 310 es
precision highp float;
precision highp int;


struct sk_struct_198999028 {
  float sk_member_635568776;
};

struct sk_struct_1343689961 {
  int sk_member_707328391;
  float sk_member_3539585619;
};

struct main_out {
  vec4 x_GLF_color_1;
};

vec4 v = vec4(0.0f);
vec4 x_GLF_color = vec4(0.0f);
layout(location = 0) out vec4 main_loc0_Output;
float tint_float_modulo(float x, float y) {
  return (x - (y * trunc((x / y))));
}
int tint_mod_i32(int lhs, int rhs) {
  uint v_1 = uint((lhs == (-2147483647 - 1)));
  bool v_2 = bool((v_1 & uint((rhs == -1))));
  uint v_3 = uint((rhs == 0));
  int v_4 = mix(rhs, 1, bool((v_3 | uint(v_2))));
  return (lhs - ((lhs / v_4) * v_4));
}
float compute_value_f1_f1_(inout float limit, inout float thirty_two) {
  float x_91 = 0.0f;
  float x_91_phi = 0.0f;
  int x_94_phi = 0;
  x_91_phi = -0.5f;
  x_94_phi = 1;
  {
    uvec2 tint_loop_idx = uvec2(4294967295u);
    while(true) {
      if (all(equal(tint_loop_idx, uvec2(0u)))) {
        break;
      }
      float x_104 = 0.0f;
      float x_113 = 0.0f;
      if ((v.w <= -0.98071229457855224609f)) {
        return 0.10000000149011611938f;
      }
      int x_95 = 0;
      float x_92_phi = 0.0f;
      x_91 = x_113;
      int x_94 = x_94_phi;
      if ((x_94 < 800)) {
      } else {
        break;
      }
      float x_112 = 0.0f;
      float x_113_phi = 0.0f;
      if ((tint_mod_i32(x_94, 32) == 0)) {
        x_104 = (x_91 + 1.0f);
        x_92_phi = x_104;
      } else {
        sk_struct_198999028 sk_struct_var_1338488225 = sk_struct_198999028(0.0f);
        sk_struct_198999028 sk_var_4041214620 = sk_struct_var_1338488225;
        float x_106 = thirty_two;
        x_113_phi = x_91;
        float v_5 = float(x_94);
        if (((v_5 - tint_float_modulo(round(x_106), floor((float(x_94) / round(x_106))))) <= 0.00999999977648258209f)) {
          x_113_phi = x_112;
        }
        x_113 = x_113_phi;
        x_92_phi = x_113;
      }
      {
        uint tint_low_inc = (tint_loop_idx.x - 1u);
        tint_loop_idx.x = tint_low_inc;
        uint tint_carry = uint((tint_low_inc == 4294967295u));
        tint_loop_idx.y = (tint_loop_idx.y - tint_carry);
        x_95 = (x_94 + 1);
        x_91_phi = x_113_phi;
        x_94_phi = x_95;
      }
      continue;
    }
  }
  return x_91;
}
void main_1() {
  sk_struct_1343689961 sk_struct_var_993442819 = sk_struct_1343689961(0, 0.0f);
  vec3 c = vec3(0.0f);
  float param = 0.0f;
  float param_1 = 0.0f;
  float x_57 = compute_value_f1_f1_(param, param_1);
}
main_out main_inner(vec4 v_6) {
  v = v_6;
  main_1();
  return main_out(x_GLF_color);
}
void main() {
  main_loc0_Output = main_inner(gl_FragCoord).x_GLF_color_1;
}
