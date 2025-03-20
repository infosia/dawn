struct main_out {
  float4 out_var_1;
};

struct main_outputs {
  float4 main_out_out_var_1 : SV_Target0;
};

struct main_inputs {
  float4 in_var_param : SV_Position;
};


static float4 in_var = (0.0f).xxxx;
static float4 out_var = (0.0f).xxxx;
void main_1() {
}

main_out main_inner(float4 in_var_param) {
  in_var = in_var_param;
  main_1();
  main_out v = {out_var};
  return v;
}

main_outputs main(main_inputs inputs) {
  main_out v_1 = main_inner(float4(inputs.in_var_param.xyz, (1.0f / inputs.in_var_param.w)));
  main_outputs v_2 = {v_1.out_var_1};
  return v_2;
}

