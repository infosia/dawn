SKIP: FAILED

struct main_out {
  float4 x_GLF_color_1;
};

struct main_outputs {
  float4 main_out_x_GLF_color_1 : SV_Target0;
};

struct main_inputs {
  float4 gl_FragCoord_param : SV_Position;
};


cbuffer cbuffer_x_7 : register(b0) {
  uint4 x_7[1];
};
cbuffer cbuffer_x_10 : register(b1) {
  uint4 x_10[4];
};
static float4 gl_FragCoord = (0.0f).xxxx;
static float4 x_GLF_color = (0.0f).xxxx;
int tint_f32_to_i32(float value) {
  return (((value <= 2147483520.0f)) ? ((((value >= -2147483648.0f)) ? (int(value)) : (int(-2147483648)))) : (int(2147483647)));
}

void main_1() {
  float4 data[2] = (float4[2])0;
  int b = int(0);
  int y = int(0);
  int i = int(0);
  float4 v = float4((asfloat(x_7[0u].x)).xxxx);
  float4 v_1[2] = {v, float4((asfloat(x_7[0u].x)).xxxx)};
  data = v_1;
  b = asint(x_10[1u].x);
  int v_2 = tint_f32_to_i32(gl_FragCoord.y);
  int v_3 = asint(x_10[1u].x);
  int v_4 = (v_3 | tint_f32_to_i32(gl_FragCoord.y));
  y = min(max(v_2, v_4), asint(x_10[1u].x));
  i = asint(x_10[1u].x);
  {
    uint2 tint_loop_idx = (4294967295u).xx;
    while(true) {
      if (all((tint_loop_idx == (0u).xx))) {
        break;
      }
      bool x_82 = false;
      bool x_83 = false;
      if ((i < asint(x_10[0u].x))) {
      } else {
        break;
      }
      bool x_76 = (b > asint(x_10[0u].x));
      x_83 = x_76;
      if (x_76) {
        x_82 = (y > asint(x_10[1u].x));
        x_83 = x_82;
      }
      if (x_83) {
        break;
      }
      b = (b + int(1));
      {
        uint tint_low_inc = (tint_loop_idx.x - 1u);
        tint_loop_idx.x = tint_low_inc;
        uint tint_carry = uint((tint_low_inc == 4294967295u));
        tint_loop_idx.y = (tint_loop_idx.y - tint_carry);
        i = (i + int(1));
      }
      continue;
    }
  }
  if ((b == asint(x_10[0u].x))) {
    int x_97 = asint(x_10[2u].x);
    int x_99 = asint(x_10[1u].x);
    int x_101 = asint(x_10[3u].x);
    uint v_5 = min(uint(min(max(x_97, x_99), x_101)), 1u);
    float v_6 = float(asint(x_10[1u].x));
    float v_7 = float(asint(x_10[2u].x));
    float v_8 = float(asint(x_10[2u].x));
    data[v_5] = float4(v_6, v_7, v_8, float(asint(x_10[1u].x)));
  }
  uint v_9 = min(uint(asint(x_10[1u].x)), 1u);
  float4 x_120 = data[v_9];
  x_GLF_color = float4(x_120.x, x_120.y, x_120.z, x_120.w);
}

main_out main_inner(float4 gl_FragCoord_param) {
  gl_FragCoord = gl_FragCoord_param;
  main_1();
  main_out v_10 = {x_GLF_color};
  return v_10;
}

main_outputs main(main_inputs inputs) {
  main_out v_11 = main_inner(float4(inputs.gl_FragCoord_param.xyz, (1.0f / inputs.gl_FragCoord_param.w)));
  main_outputs v_12 = {v_11.x_GLF_color_1};
  return v_12;
}

