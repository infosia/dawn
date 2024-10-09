struct modf_result_f32 {
  float fract;
  float whole;
};


[numthreads(1, 1, 1)]
void main() {
  float runtime_in = 1.25f;
  modf_result_f32 res = {0.25f, 1.0f};
  float v = 0.0f;
  float v_1 = modf(runtime_in, v);
  modf_result_f32 v_2 = {v_1, v};
  modf_result_f32 v_3 = v_2;
  res = v_3;
  modf_result_f32 v_4 = {0.25f, 1.0f};
  res = v_4;
  float fract = res.fract;
  float whole = res.whole;
}

