#version 310 es


struct mat3x3_f32_std140 {
  vec3 col0;
  vec3 col1;
  vec3 col2;
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  mat3x3_f32_std140 tint_symbol[4];
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  float tint_symbol_2;
} v_1;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec3 v_2 = v.tint_symbol[2].col0;
  vec3 v_3 = v.tint_symbol[2].col1;
  mat3 v_4 = mat3(v_2, v_3, v.tint_symbol[2].col2);
  mat3x3_f32_std140 v_5[4] = v.tint_symbol;
  mat3 v_6[4] = mat3[4](mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)));
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      v_6[v_8] = mat3(v_5[v_8].col0, v_5[v_8].col1, v_5[v_8].col2);
      {
        v_7 = (v_8 + 1u);
      }
      continue;
    }
  }
  mat3 l_a[4] = v_6;
  mat3 l_a_i = v_4;
  vec3 l_a_i_i = v_4[1];
  v_1.tint_symbol_2 = (((v_4[1][0u] + l_a[0][0][0u]) + l_a_i[0][0u]) + l_a_i_i[0u]);
}
