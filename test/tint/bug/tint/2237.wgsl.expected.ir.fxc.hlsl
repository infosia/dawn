
RWByteAddressBuffer buffer : register(u0);
uint foo() {
  uint v_1 = buffer.Load(0u);
  uint v_2[4] = {0u, 1u, 2u, 4u};
  return v_2[v_1];
}

[numthreads(1, 1, 1)]
void main() {
  uint v_3 = buffer.Load(0u);
  uint v_4[4] = {0u, 1u, 2u, 4u};
  uint v = v_4[v_3];
  buffer.Store(0u, (v + foo()));
}

