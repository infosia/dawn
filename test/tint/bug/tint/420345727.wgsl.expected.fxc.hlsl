float tint_trunc(float param_0) {
  return param_0 < 0 ? ceil(param_0) : floor(param_0);
}

struct sk_struct_1343689961 {
  int sk_member_707328391;
  float sk_member_3539585619;
};
struct sk_struct_198999028 {
  float sk_member_635568776;
};

static float4 gl_FragCoord = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 x_GLF_color = float4(0.0f, 0.0f, 0.0f, 0.0f);

int tint_mod(int lhs, int rhs) {
  int rhs_or_one = (((rhs == 0) | ((lhs == -2147483648) & (rhs == -1))) ? 1 : rhs);
  if (any(((uint((lhs | rhs_or_one)) & 2147483648u) != 0u))) {
    return (lhs - ((lhs / rhs_or_one) * rhs_or_one));
  } else {
    return (lhs % rhs_or_one);
  }
}

float tint_float_mod(float lhs, float rhs) {
  return (lhs - (tint_trunc((lhs / rhs)) * rhs));
}

float compute_value_f1_f1_(inout float limit, inout float thirty_two) {
  float x_91 = 0.0f;
  float x_91_phi = 0.0f;
  int x_94_phi = 0;
  x_91_phi = -0.5f;
  x_94_phi = 1;
  while (true) {
    float x_104 = 0.0f;
    float x_113 = 0.0f;
    if ((gl_FragCoord.w <= -0.98071229457855224609f)) {
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
    if ((tint_mod(x_94, 32) == 0)) {
      x_104 = (x_91 + 1.0f);
      x_92_phi = x_104;
    } else {
      sk_struct_198999028 sk_struct_var_1338488225 = (sk_struct_198999028)0;
      sk_struct_198999028 sk_var_4041214620 = sk_struct_var_1338488225;
      float x_106 = thirty_two;
      x_113_phi = x_91;
      if (((float(x_94) - tint_float_mod(round(x_106), floor((float(x_94) / round(x_106))))) <= 0.00999999977648258209f)) {
        x_113_phi = x_112;
      }
      x_113 = x_113_phi;
      x_92_phi = x_113;
    }
    {
      x_95 = (x_94 + 1);
      x_91_phi = x_113_phi;
      x_94_phi = x_95;
    }
  }
  return x_91;
}

void main_1() {
  sk_struct_1343689961 sk_struct_var_993442819 = (sk_struct_1343689961)0;
  float3 c = float3(0.0f, 0.0f, 0.0f);
  float param = 0.0f;
  float param_1 = 0.0f;
  float x_57 = compute_value_f1_f1_(param, param_1);
  return;
}

struct main_out {
  float4 x_GLF_color_1;
};
struct tint_symbol_1 {
  float4 gl_FragCoord_param : SV_Position;
};
struct tint_symbol_2 {
  float4 x_GLF_color_1 : SV_Target0;
};

main_out main_inner(float4 gl_FragCoord_param) {
  gl_FragCoord = gl_FragCoord_param;
  main_1();
  main_out tint_symbol_3 = {x_GLF_color};
  return tint_symbol_3;
}

tint_symbol_2 main(tint_symbol_1 tint_symbol) {
  main_out inner_result = main_inner(float4(tint_symbol.gl_FragCoord_param.xyz, (1.0f / tint_symbol.gl_FragCoord_param.w)));
  tint_symbol_2 wrapper_result = (tint_symbol_2)0;
  wrapper_result.x_GLF_color_1 = inner_result.x_GLF_color_1;
  return wrapper_result;
}
