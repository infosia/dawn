#version 310 es
precision highp float;
precision highp int;


struct S {
  float a;
};

layout(binding = 0, std430)
buffer f_b0_block_ssbo {
  S inner;
} v;
layout(binding = 1, std430)
buffer f_b1_block_ssbo {
  S inner;
} v_1;
layout(binding = 2, std430)
buffer f_b2_block_ssbo {
  S inner;
} v_2;
layout(binding = 3, std430)
buffer f_b3_block_ssbo {
  S inner;
} v_3;
layout(binding = 4, std430)
buffer f_b4_block_ssbo {
  S inner;
} v_4;
layout(binding = 5, std430)
buffer f_b5_block_ssbo {
  S inner;
} v_5;
layout(binding = 6, std430)
buffer f_b6_block_ssbo {
  S inner;
} v_6;
layout(binding = 7, std430)
buffer f_b7_block_ssbo {
  S inner;
} v_7;
layout(binding = 8, std140)
uniform f_b8_block_ubo {
  uvec4 inner[1];
} v_8;
layout(binding = 9, std140)
uniform f_b9_block_ubo {
  uvec4 inner[1];
} v_9;
layout(binding = 10, std140)
uniform f_b10_block_ubo {
  uvec4 inner[1];
} v_10;
layout(binding = 11, std140)
uniform f_b11_block_ubo {
  uvec4 inner[1];
} v_11;
layout(binding = 12, std140)
uniform f_b12_block_ubo {
  uvec4 inner[1];
} v_12;
layout(binding = 13, std140)
uniform f_b13_block_ubo {
  uvec4 inner[1];
} v_13;
layout(binding = 14, std140)
uniform f_b14_block_ubo {
  uvec4 inner[1];
} v_14;
layout(binding = 15, std140)
uniform f_b15_block_ubo {
  uvec4 inner[1];
} v_15;
uniform highp sampler2D f_t0;
uniform highp sampler2D f_t1;
uniform highp sampler2D f_t2;
uniform highp sampler2D f_t3;
uniform highp sampler2D f_t4;
uniform highp sampler2D f_t5;
uniform highp sampler2D f_t6;
uniform highp sampler2D f_t7;
uniform highp sampler2DShadow f_t8;
uniform highp sampler2DShadow f_t9;
uniform highp sampler2DShadow f_t10;
uniform highp sampler2DShadow f_t11;
uniform highp sampler2DShadow f_t12;
uniform highp sampler2DShadow f_t13;
uniform highp sampler2DShadow f_t14;
uniform highp sampler2DShadow f_t15;
S v_16(uint start_byte_offset) {
  return S(uintBitsToFloat(v_15.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_17(uint start_byte_offset) {
  return S(uintBitsToFloat(v_14.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_18(uint start_byte_offset) {
  return S(uintBitsToFloat(v_13.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_19(uint start_byte_offset) {
  return S(uintBitsToFloat(v_12.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_20(uint start_byte_offset) {
  return S(uintBitsToFloat(v_11.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_21(uint start_byte_offset) {
  return S(uintBitsToFloat(v_10.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_22(uint start_byte_offset) {
  return S(uintBitsToFloat(v_9.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
S v_23(uint start_byte_offset) {
  return S(uintBitsToFloat(v_8.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]));
}
void main() {
  v_23(0u);
  v_22(0u);
  v_21(0u);
  v_20(0u);
  v_19(0u);
  v_18(0u);
  v_17(0u);
  v_16(0u);
}
