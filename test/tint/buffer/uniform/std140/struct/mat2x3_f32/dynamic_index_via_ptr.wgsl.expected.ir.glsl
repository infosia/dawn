#version 310 es


struct Inner_std140 {
  vec3 m_col0;
  vec3 m_col1;
};

struct Outer_std140 {
  Inner_std140 a[4];
};

struct Inner {
  mat2x3 m;
};

struct Outer {
  Inner a[4];
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  Outer_std140 tint_symbol[4];
} v;
int counter = 0;
int i() {
  counter = (counter + 1);
  return counter;
}
Inner tint_convert_Inner(Inner_std140 tint_input) {
  return Inner(mat2x3(tint_input.m_col0, tint_input.m_col1));
}
Outer tint_convert_Outer(Outer_std140 tint_input) {
  Inner v_1[4] = Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))));
  {
    uint v_2 = 0u;
    v_2 = 0u;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 4u)) {
        break;
      }
      v_1[v_3] = tint_convert_Inner(tint_input.a[v_3]);
      {
        v_2 = (v_3 + 1u);
      }
      continue;
    }
  }
  return Outer(v_1);
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  mat2x3 v_4 = mat2x3(v.tint_symbol[i()].a[i()].m_col0, v.tint_symbol[i()].a[i()].m_col1);
  vec3 v_5 = v_4[i()];
  Outer_std140 v_6[4] = v.tint_symbol;
  Outer v_7[4] = Outer[4](Outer(Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))))));
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      v_7[v_9] = tint_convert_Outer(v_6[v_9]);
      {
        v_8 = (v_9 + 1u);
      }
      continue;
    }
  }
  Outer l_a[4] = v_7;
  Outer l_a_i = tint_convert_Outer(v.tint_symbol[i()]);
  Inner_std140 v_10[4] = v.tint_symbol[i()].a;
  Inner v_11[4] = Inner[4](Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))), Inner(mat2x3(vec3(0.0f), vec3(0.0f))));
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      v_11[v_13] = tint_convert_Inner(v_10[v_13]);
      {
        v_12 = (v_13 + 1u);
      }
      continue;
    }
  }
  Inner l_a_i_a[4] = v_11;
  Inner l_a_i_a_i = tint_convert_Inner(v.tint_symbol[i()].a[i()]);
  mat2x3 l_a_i_a_i_m = v_4;
  vec3 l_a_i_a_i_m_i = v_5;
  float l_a_i_a_i_m_i_i = v_5[i()];
}
