#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct Inner_std140 {
  f16vec4 m_col0;
  f16vec4 m_col1;
  f16vec4 m_col2;
};

struct Outer_std140 {
  Inner_std140 a[4];
};

struct Inner {
  f16mat3x4 m;
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
  return Inner(f16mat3x4(tint_input.m_col0, tint_input.m_col1, tint_input.m_col2));
}
Outer tint_convert_Outer(Outer_std140 tint_input) {
  Inner v_1[4] = Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))));
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
  int v_4 = i();
  int v_5 = i();
  f16vec4 v_6 = v.tint_symbol[v_4].a[v_5].m_col0;
  f16vec4 v_7 = v.tint_symbol[v_4].a[v_5].m_col1;
  f16mat3x4 v_8 = f16mat3x4(v_6, v_7, v.tint_symbol[v_4].a[v_5].m_col2);
  f16vec4 v_9 = v_8[i()];
  Outer_std140 v_10[4] = v.tint_symbol;
  Outer v_11[4] = Outer[4](Outer(Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))))), Outer(Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))))), Outer(Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))))), Outer(Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))))));
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      v_11[v_13] = tint_convert_Outer(v_10[v_13]);
      {
        v_12 = (v_13 + 1u);
      }
      continue;
    }
  }
  Outer l_a[4] = v_11;
  Outer l_a_i = tint_convert_Outer(v.tint_symbol[v_4]);
  Inner_std140 v_14[4] = v.tint_symbol[v_4].a;
  Inner v_15[4] = Inner[4](Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))), Inner(f16mat3x4(f16vec4(0.0hf), f16vec4(0.0hf), f16vec4(0.0hf))));
  {
    uint v_16 = 0u;
    v_16 = 0u;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      v_15[v_17] = tint_convert_Inner(v_14[v_17]);
      {
        v_16 = (v_17 + 1u);
      }
      continue;
    }
  }
  Inner l_a_i_a[4] = v_15;
  Inner l_a_i_a_i = tint_convert_Inner(v.tint_symbol[v_4].a[v_5]);
  f16mat3x4 l_a_i_a_i_m = v_8;
  f16vec4 l_a_i_a_i_m_i = v_9;
  float16_t l_a_i_a_i_m_i_i = v_9[i()];
}
