static float4 in_var = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 out_var = float4(0.0f, 0.0f, 0.0f, 0.0f);

void main_1() {
  return;
}

struct main_out {
  float4 out_var_1;
};
struct tint_symbol_1 {
  float4 in_var_param : SV_Position;
};
struct tint_symbol_2 {
  float4 out_var_1 : SV_Target0;
};

main_out main_inner(float4 in_var_param) {
  in_var = in_var_param;
  main_1();
  main_out tint_symbol_3 = {out_var};
  return tint_symbol_3;
}

tint_symbol_2 main(tint_symbol_1 tint_symbol) {
  main_out inner_result = main_inner(float4(tint_symbol.in_var_param.xyz, (1.0f / tint_symbol.in_var_param.w)));
  tint_symbol_2 wrapper_result = (tint_symbol_2)0;
  wrapper_result.out_var_1 = inner_result.out_var_1;
  return wrapper_result;
}
