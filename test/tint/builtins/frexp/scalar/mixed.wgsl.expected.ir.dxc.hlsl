struct frexp_result_f32 {
  float fract;
  int exp;
};


[numthreads(1, 1, 1)]
void main() {
  float runtime_in = 1.25f;
  frexp_result_f32 res = {0.625f, int(1)};
  float v = 0.0f;
  float v_1 = frexp(runtime_in, v);
  float v_2 = float(sign(runtime_in));
  v = (v_2 * v);
  frexp_result_f32 v_3 = {v_1, int(v)};
  frexp_result_f32 v_4 = v_3;
  res = v_4;
  frexp_result_f32 v_5 = {0.625f, int(1)};
  res = v_5;
  float fract = res.fract;
  int exp = res.exp;
}

