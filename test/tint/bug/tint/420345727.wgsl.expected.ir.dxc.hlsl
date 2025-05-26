struct sk_struct_198999028 {
  float sk_member_635568776;
};

struct sk_struct_1343689961 {
  int sk_member_707328391;
  float sk_member_3539585619;
};

struct main_out {
  float4 x_GLF_color_1;
};

struct main_outputs {
  float4 main_out_x_GLF_color_1 : SV_Target0;
};

struct main_inputs {
  float4 gl_FragCoord_param : SV_Position;
};


static float4 gl_FragCoord = (0.0f).xxxx;
static float4 x_GLF_color = (0.0f).xxxx;
int tint_mod_i32(int lhs, int rhs) {
  int v = ((((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1))))) ? (int(1)) : (rhs));
  return (lhs - ((lhs / v) * v));
}

float compute_value_f1_f1_(inout float limit, inout float thirty_two) {
  float x_91 = 0.0f;
  float x_91_phi = 0.0f;
  int x_94_phi = int(0);
  x_91_phi = -0.5f;
  x_94_phi = int(1);
  {
    uint2 tint_loop_idx = (4294967295u).xx;
    while(true) {
      if (all((tint_loop_idx == (0u).xx))) {
        break;
      }
      float x_104 = 0.0f;
      float x_113 = 0.0f;
      if ((gl_FragCoord.w <= -0.98071229457855224609f)) {
        return 0.10000000149011611938f;
      }
      int x_95 = int(0);
      float x_92_phi = 0.0f;
      x_91 = x_113;
      int x_94 = x_94_phi;
      if ((x_94 < int(800))) {
      } else {
        break;
      }
      float x_112 = 0.0f;
      float x_113_phi = 0.0f;
      if ((tint_mod_i32(x_94, int(32)) == int(0))) {
        x_104 = (x_91 + 1.0f);
        x_92_phi = x_104;
      } else {
        sk_struct_198999028 sk_struct_var_1338488225 = (sk_struct_198999028)0;
        sk_struct_198999028 sk_var_4041214620 = sk_struct_var_1338488225;
        float x_106 = thirty_two;
        x_113_phi = x_91;
        float v_1 = float(x_94);
        float v_2 = floor((float(x_94) / round(x_106)));
        float v_3 = (round(x_106) / v_2);
        if (((v_1 - (round(x_106) - ((((v_3 < 0.0f)) ? (ceil(v_3)) : (floor(v_3))) * v_2))) <= 0.00999999977648258209f)) {
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
        x_95 = (x_94 + int(1));
        x_91_phi = x_113_phi;
        x_94_phi = x_95;
      }
      continue;
    }
  }
  return x_91;
}

void main_1() {
  sk_struct_1343689961 sk_struct_var_993442819 = (sk_struct_1343689961)0;
  float3 c = (0.0f).xxx;
  float param = 0.0f;
  float param_1 = 0.0f;
  float x_57 = compute_value_f1_f1_(param, param_1);
}

main_out main_inner(float4 gl_FragCoord_param) {
  gl_FragCoord = gl_FragCoord_param;
  main_1();
  main_out v_4 = {x_GLF_color};
  return v_4;
}

main_outputs main(main_inputs inputs) {
  main_out v_5 = main_inner(float4(inputs.gl_FragCoord_param.xyz, (1.0f / inputs.gl_FragCoord_param.w)));
  main_outputs v_6 = {v_5.x_GLF_color_1};
  return v_6;
}

