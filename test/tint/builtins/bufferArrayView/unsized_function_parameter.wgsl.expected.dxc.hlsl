
RWByteAddressBuffer v : register(u0);
void foo(uint tint_array_length) {
  v.Store(((4u + (select((tint_array_length < 32u), 0u, 0u) * 1u)) + (min(uint(int(0)), ((select((tint_array_length < 32u), 8u, 32u) / 8u) - 1u)) * 8u)), asuint(3.0f));
}

[numthreads(1, 1, 1)]
void main() {
  uint v_1 = 0u;
  v.GetDimensions(v_1);
  foo(v_1);
}

