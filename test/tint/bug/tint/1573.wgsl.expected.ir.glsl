#version 310 es


struct atomic_compare_exchange_result_u32 {
  uint old_value;
  bool exchanged;
};

layout(binding = 0, std430)
buffer tint_symbol_2_1_ssbo {
  uint tint_symbol_1;
} v;
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint value = 42u;
  uint v_1 = value;
  uint v_2 = atomicCompSwap(v.tint_symbol_1, 0u, v_1);
  atomic_compare_exchange_result_u32 result = atomic_compare_exchange_result_u32(v_2, (v_2 == 0u));
}
