#version 310 es


struct Inner_std140 {
  vec3 m_col0;
  vec3 m_col1;
  vec3 m_col2;
};

struct Outer_std140 {
  Inner_std140 a[4];
};

struct Inner {
  mat3 m;
};

struct Outer {
  Inner a[4];
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  Outer_std140 tint_symbol[4];
} v;
Inner tint_convert_Inner(Inner_std140 tint_input) {
  return Inner(mat3(tint_input.m_col0, tint_input.m_col1, tint_input.m_col2));
}
Outer tint_convert_Outer(Outer_std140 tint_input) {
  Inner v_1[4] = Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))));
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
  vec3 v_4 = v.tint_symbol[3].a[2].m_col0;
  vec3 v_5 = v.tint_symbol[3].a[2].m_col1;
  mat3 v_6 = mat3(v_4, v_5, v.tint_symbol[3].a[2].m_col2);
  Outer_std140 v_7[4] = v.tint_symbol;
  Outer v_8[4] = Outer[4](Outer(Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))))), Outer(Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))))));
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      v_8[v_10] = tint_convert_Outer(v_7[v_10]);
      {
        v_9 = (v_10 + 1u);
      }
      continue;
    }
  }
  Outer l_a[4] = v_8;
  Outer l_a_3 = tint_convert_Outer(v.tint_symbol[3]);
  Inner_std140 v_11[4] = v.tint_symbol[3].a;
  Inner v_12[4] = Inner[4](Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))), Inner(mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f))));
  {
    uint v_13 = 0u;
    v_13 = 0u;
    while(true) {
      uint v_14 = v_13;
      if ((v_14 >= 4u)) {
        break;
      }
      v_12[v_14] = tint_convert_Inner(v_11[v_14]);
      {
        v_13 = (v_14 + 1u);
      }
      continue;
    }
  }
  Inner l_a_3_a[4] = v_12;
  Inner l_a_3_a_2 = tint_convert_Inner(v.tint_symbol[3].a[2]);
  mat3 l_a_3_a_2_m = v_6;
  vec3 l_a_3_a_2_m_1 = v_6[1];
  float l_a_3_a_2_m_1_0 = v_6[1][0];
}
