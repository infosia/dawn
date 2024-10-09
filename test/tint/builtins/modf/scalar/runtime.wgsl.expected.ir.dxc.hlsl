struct modf_result_f32 {
  float fract;
  float whole;
};


[numthreads(1, 1, 1)]
void main() {
  float tint_symbol = 1.25f;
  float v = 0.0f;
  float v_1 = modf(tint_symbol, v);
  modf_result_f32 v_2 = {v_1, v};
  modf_result_f32 v_3 = v_2;
  modf_result_f32 res = v_3;
  float fract = res.fract;
  modf_result_f32 v_4 = v_3;
  float whole = v_4.whole;
}

