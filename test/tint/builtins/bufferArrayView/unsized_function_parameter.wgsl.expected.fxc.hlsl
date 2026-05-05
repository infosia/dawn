
RWByteAddressBuffer v : register(u0);
void foo(uint tint_array_length) {
  uint v_1 = (((tint_array_length < 32u)) ? (0u) : (0u));
  uint v_2 = (((((tint_array_length < 32u)) ? (8u) : (32u)) / 8u) - 1u);
  v.Store(((4u + (v_1 * 1u)) + (min(uint(int(0)), v_2) * 8u)), asuint(3.0f));
}

[numthreads(1, 1, 1)]
void main() {
  uint v_3 = 0u;
  v.GetDimensions(v_3);
  foo(v_3);
}

