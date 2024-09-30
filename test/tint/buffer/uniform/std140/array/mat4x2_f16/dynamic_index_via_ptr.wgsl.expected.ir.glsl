#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct mat4x2_f16_std140 {
  f16vec2 col0;
  f16vec2 col1;
  f16vec2 col2;
  f16vec2 col3;
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  mat4x2_f16_std140 tint_symbol[4];
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  float16_t tint_symbol_2;
} v_1;
int counter = 0;
int i() {
  counter = (counter + 1);
  return counter;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  int v_2 = i();
  f16vec2 v_3 = v.tint_symbol[v_2].col0;
  f16vec2 v_4 = v.tint_symbol[v_2].col1;
  f16vec2 v_5 = v.tint_symbol[v_2].col2;
  f16mat4x2 v_6 = f16mat4x2(v_3, v_4, v_5, v.tint_symbol[v_2].col3);
  f16vec2 v_7 = v_6[i()];
  mat4x2_f16_std140 v_8[4] = v.tint_symbol;
  f16mat4x2 v_9[4] = f16mat4x2[4](f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)), f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)), f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)), f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)));
  {
    uint v_10 = 0u;
    v_10 = 0u;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 4u)) {
        break;
      }
      v_9[v_11] = f16mat4x2(v_8[v_11].col0, v_8[v_11].col1, v_8[v_11].col2, v_8[v_11].col3);
      {
        v_10 = (v_11 + 1u);
      }
      continue;
    }
  }
  f16mat4x2 l_a[4] = v_9;
  f16mat4x2 l_a_i = v_6;
  f16vec2 l_a_i_i = v_7;
  v_1.tint_symbol_2 = (((v_7[0u] + l_a[0][0][0u]) + l_a_i[0][0u]) + l_a_i_i[0u]);
}
