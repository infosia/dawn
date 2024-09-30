#version 310 es


struct atomic_compare_exchange_result_u32 {
  uint old_value;
  bool exchanged;
};

struct atomic_compare_exchange_result_i32 {
  int old_value;
  bool exchanged;
};

layout(binding = 0, std430)
buffer tint_symbol_2_1_ssbo {
  uint tint_symbol_1;
} v;
layout(binding = 1, std430)
buffer tint_symbol_4_1_ssbo {
  int tint_symbol_3;
} v_1;
shared uint b_u32;
shared int b_i32;
void tint_symbol_inner(uint tint_local_index) {
  if ((tint_local_index == 0u)) {
    atomicExchange(b_u32, 0u);
    atomicExchange(b_i32, 0);
  }
  barrier();
  uint value = 42u;
  uint v_2 = value;
  uint v_3 = atomicCompSwap(v.tint_symbol_1, 0u, v_2);
  atomic_compare_exchange_result_u32 r1 = atomic_compare_exchange_result_u32(v_3, (v_3 == 0u));
  uint v_4 = value;
  uint v_5 = atomicCompSwap(v.tint_symbol_1, 0u, v_4);
  atomic_compare_exchange_result_u32 r2 = atomic_compare_exchange_result_u32(v_5, (v_5 == 0u));
  uint v_6 = value;
  uint v_7 = atomicCompSwap(v.tint_symbol_1, 0u, v_6);
  atomic_compare_exchange_result_u32 r3 = atomic_compare_exchange_result_u32(v_7, (v_7 == 0u));
  int value_1 = 42;
  int v_8 = value_1;
  int v_9 = atomicCompSwap(v_1.tint_symbol_3, 0, v_8);
  atomic_compare_exchange_result_i32 r1_1 = atomic_compare_exchange_result_i32(v_9, (v_9 == 0));
  int v_10 = value_1;
  int v_11 = atomicCompSwap(v_1.tint_symbol_3, 0, v_10);
  atomic_compare_exchange_result_i32 r2_1 = atomic_compare_exchange_result_i32(v_11, (v_11 == 0));
  int v_12 = value_1;
  int v_13 = atomicCompSwap(v_1.tint_symbol_3, 0, v_12);
  atomic_compare_exchange_result_i32 r3_1 = atomic_compare_exchange_result_i32(v_13, (v_13 == 0));
  uint value_2 = 42u;
  uint v_14 = atomicCompSwap(b_u32, 0u, value_2);
  atomic_compare_exchange_result_u32 r1_2 = atomic_compare_exchange_result_u32(v_14, (v_14 == 0u));
  uint v_15 = atomicCompSwap(b_u32, 0u, value_2);
  atomic_compare_exchange_result_u32 r2_2 = atomic_compare_exchange_result_u32(v_15, (v_15 == 0u));
  uint v_16 = atomicCompSwap(b_u32, 0u, value_2);
  atomic_compare_exchange_result_u32 r3_2 = atomic_compare_exchange_result_u32(v_16, (v_16 == 0u));
  int value_3 = 42;
  int v_17 = atomicCompSwap(b_i32, 0, value_3);
  atomic_compare_exchange_result_i32 r1_3 = atomic_compare_exchange_result_i32(v_17, (v_17 == 0));
  int v_18 = atomicCompSwap(b_i32, 0, value_3);
  atomic_compare_exchange_result_i32 r2_3 = atomic_compare_exchange_result_i32(v_18, (v_18 == 0));
  int v_19 = atomicCompSwap(b_i32, 0, value_3);
  atomic_compare_exchange_result_i32 r3_3 = atomic_compare_exchange_result_i32(v_19, (v_19 == 0));
}
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_symbol_inner(gl_LocalInvocationIndex);
}
