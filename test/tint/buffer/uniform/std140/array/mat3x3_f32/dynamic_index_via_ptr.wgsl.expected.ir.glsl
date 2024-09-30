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
int counter = 0;
int i() {
  counter = (counter + 1);
  return counter;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  int v_2 = i();
  vec3 v_3 = v.tint_symbol[v_2].col0;
  vec3 v_4 = v.tint_symbol[v_2].col1;
  mat3 v_5 = mat3(v_3, v_4, v.tint_symbol[v_2].col2);
  mat3x3_f32_std140 v_6[4] = v.tint_symbol;
  mat3 v_7[4] = mat3[4](mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)), mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f)));
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      v_7[v_9] = mat3(v_6[v_9].col0, v_6[v_9].col1, v_6[v_9].col2);
      {
        v_8 = (v_9 + 1u);
      }
      continue;
    }
  }
  mat3 l_a[4] = v_7;
  mat3 l_a_i = v_5;
  vec3 l_a_i_i = v_5[i()];
  v_1.tint_symbol_2 = (((v_5[i()][0u] + l_a[0][0][0u]) + l_a_i[0][0u]) + l_a_i_i[0u]);
}
